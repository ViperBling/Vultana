#include "RHISwapchainVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHITextureVK.hpp"
#include "RHI/RHICommon.hpp"
#include "Utilities/Log.hpp"
#include "Window/GLFWindow.hpp"

namespace RHI
{
    RHISwapchainVK::RHISwapchainVK(RHIDeviceVK *device, const RHISwapchainDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Desc = desc;
        m_Name = name;
    }

    RHISwapchainVK::~RHISwapchainVK()
    {
        for (size_t i = 0; i < m_BackBuffers.size(); i++)
        {
            delete m_BackBuffers[i];
        }
        m_BackBuffers.clear();

        auto device = (RHIDeviceVK*)m_pDevice;
        device->Delete(m_Swapchain);
        device->Delete(m_Surface);

        for (size_t i = 0; i < m_AcquireSemaphores.size(); i++)
        {
            device->Delete(m_AcquireSemaphores[i]);
        }
        for (size_t i = 0; i < m_PresentSemaphores.size(); i++)
        {
            device->Delete(m_PresentSemaphores[i]);
        }
    }

    bool RHISwapchainVK::Create()
    {
        if (!CreateSurface() 
            || !CreateSwapchain() 
            || !CreateTextures() 
            || !CreateSemaphores()
            )
        {
            VTNA_LOG_ERROR("[RHISwapchainVK] Failed to create {}", m_Name);
            return false;
        }
        return true;
    }

    void RHISwapchainVK::Present(vk::Queue queue)
    {
        vk::Semaphore waitSemphore = GetPresentSemaphore();

        vk::PresentInfoKHR presentInfo {};
        presentInfo.setWaitSemaphores(waitSemphore);
        presentInfo.setSwapchains(m_Swapchain);
        presentInfo.setImageIndices(m_CurrentBackBuffer);

        vk::Result res = queue.presentKHR(presentInfo);

        if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapchain();
        }
    }

    vk::Semaphore RHISwapchainVK::GetAcquireSemaphore()
    {
        return m_AcquireSemaphores[m_FrameSemaphoreIndex];
    }

    vk::Semaphore RHISwapchainVK::GetPresentSemaphore()
    {
        return m_PresentSemaphores[m_FrameSemaphoreIndex];
    }

    void RHISwapchainVK::AcquireNextBackBuffer()
    {
        m_FrameSemaphoreIndex = (m_FrameSemaphoreIndex + 1) % m_AcquireSemaphores.size();

        vk::Semaphore signalSemphore = GetAcquireSemaphore();

        vk::Result res = ((RHIDeviceVK*)m_pDevice)->GetDevice().acquireNextImageKHR(m_Swapchain, UINT64_MAX, signalSemphore, nullptr, &m_CurrentBackBuffer);

        if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapchain();

            res = ((RHIDeviceVK*)m_pDevice)->GetDevice().acquireNextImageKHR(m_Swapchain, UINT64_MAX, signalSemphore, nullptr, &m_CurrentBackBuffer);
            assert(res == vk::Result::eSuccess);
        }
    }

    RHITexture *RHISwapchainVK::GetBackBuffer() const
    {
        return m_BackBuffers[m_CurrentBackBuffer];
    }

    bool RHISwapchainVK::Resize(uint32_t width, uint32_t height)
    {
        if (m_Desc.Width == width && m_Desc.Height == height)
        {
            return false;
        }
        m_Desc.Width = width;
        m_Desc.Height = height;
        return RecreateSwapchain();
    }

    void RHISwapchainVK::SetVSyncEnabled(bool enabled)
    {
        if (m_bEnableVSync != enabled)
        {
            m_bEnableVSync = enabled;
            RecreateSwapchain();
        }
    }

    bool RHISwapchainVK::CreateSurface()
    {
        vk::Instance instance = ((RHIDeviceVK*)m_pDevice)->GetInstance();
        vk::Device device = ((RHIDeviceVK*)m_pDevice)->GetDevice();
        vk::detail::DispatchLoaderDynamic dynamicLoader = ((RHIDeviceVK*)m_pDevice)->GetDynamicLoader();
        vk::PhysicalDevice physcialDevice = ((RHIDeviceVK*)m_pDevice)->GetPhysicalDevice();
        
        vk::Win32SurfaceCreateInfoKHR surfaceCI {};
        surfaceCI.hinstance = GetModuleHandle(nullptr);
        surfaceCI.hwnd = (HWND)m_Desc.WindowHandle;
        // auto wnd = (Window::GLFWindow*)m_Desc.WindowHandle;
        // vk::Result res = wnd->CreateVulkanSurface(instance, m_Surface);
        m_Surface = instance.createWin32SurfaceKHR(surfaceCI, nullptr);

        SetDebugName(device, vk::ObjectType::eSurfaceKHR, (uint64_t)(VkSurfaceKHR)m_Surface, m_Name.c_str(), dynamicLoader);

        auto presentMode = physcialDevice.getSurfacePresentModesKHR(m_Surface);
        m_bMailboxSupported = eastl::find(presentMode.begin(), presentMode.end(), vk::PresentModeKHR::eMailbox) != presentMode.end();

        return true;
    }

    bool RHISwapchainVK::CreateSwapchain()
    {
        vk::Device device = ((RHIDeviceVK*)m_pDevice)->GetDevice();
        auto dynamicLoader = ((RHIDeviceVK*)m_pDevice)->GetDynamicLoader();
        auto physcialDevice = ((RHIDeviceVK*)m_pDevice)->GetPhysicalDevice();
        vk::SwapchainKHR oldSwapchain = m_Swapchain;

        auto presentMode = physcialDevice.getSurfacePresentModesKHR(m_Surface);
        auto surfaceCaps = physcialDevice.getSurfaceCapabilitiesKHR(m_Surface);

        // vk::Extent2D extent = vk::Extent2D(
        //     eastl::clamp(m_Desc.Width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width),
        //     eastl::clamp(m_Desc.Height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height)
        // );
        vk::Extent2D extent = vk::Extent2D(m_Desc.Width, m_Desc.Height);

        vk::Format viewFormats[2];
        viewFormats[0] = ToVulkanFormat(m_Desc.ColorFormat);
        viewFormats[1] = ToVulkanFormat(m_Desc.ColorFormat, true);

        vk::ImageFormatListCreateInfo imageFormatListCI {};
        imageFormatListCI.setViewFormats(viewFormats);

        vk::SwapchainCreateInfoKHR swapchainCI {};
        swapchainCI.setSurface(m_Surface);
        swapchainCI.setMinImageCount(m_Desc.BufferCount);
        swapchainCI.setImageFormat(ToVulkanFormat(m_Desc.ColorFormat));
        swapchainCI.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
        swapchainCI.setImageExtent(extent);
        swapchainCI.setImageArrayLayers(1);
        swapchainCI.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        swapchainCI.setImageSharingMode(vk::SharingMode::eExclusive);
        swapchainCI.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapchainCI.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapchainCI.setPresentMode(m_bEnableVSync ? vk::PresentModeKHR::eFifo : (m_bMailboxSupported ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eImmediate));
        swapchainCI.setClipped(true);
        swapchainCI.setOldSwapchain(oldSwapchain);

        if (IsSRGBFormat(m_Desc.ColorFormat))
        {
            swapchainCI.setFlags(vk::SwapchainCreateFlagBitsKHR::eMutableFormat);
            swapchainCI.setPNext(&imageFormatListCI);
        }

        vk::Result res = device.createSwapchainKHR(&swapchainCI, nullptr, &m_Swapchain);
        if (res != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHISwapchainVK] Failed to create swapchain");
            return false;
        }
        SetDebugName(device, vk::ObjectType::eSwapchainKHR, (uint64_t)(VkSwapchainKHR)m_Swapchain, m_Name.c_str(), dynamicLoader);

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            ((RHIDeviceVK*)m_pDevice)->Delete(oldSwapchain);
        }
        return true;
    }

    bool RHISwapchainVK::CreateTextures()
    {
        vk::Device device = (VkDevice)m_pDevice->GetNativeHandle();

        RHI::RHITextureDesc desc {};
        desc.Width = m_Desc.Width;
        desc.Height = m_Desc.Height;
        desc.Format = m_Desc.ColorFormat;
        desc.Usage = RHITextureUsageRenderTarget;

        auto images = device.getSwapchainImagesKHR(m_Swapchain);

        for (uint32_t i = 0; i < images.size(); i++)
        {
            eastl::string name = fmt::format("{} texture {}", m_Name, i).c_str();

            RHITextureVK* texture = new RHITextureVK((RHIDeviceVK*)m_pDevice, desc, name);
            texture->Create(images[i]);
            
            m_BackBuffers.push_back(texture);
        }
        return true;
    }

    bool RHISwapchainVK::CreateSemaphores()
    {
        vk::Device device = ((RHIDeviceVK*)m_pDevice)->GetDevice();
        auto dynamicLoader = ((RHIDeviceVK*)m_pDevice)->GetDynamicLoader();
        vk::SemaphoreCreateInfo semaphoreCI {};

        for (uint32_t i = 0; i < m_Desc.BufferCount; i++)
        {
            vk::Semaphore semaphore;

            vk::Result res = device.createSemaphore(&semaphoreCI, nullptr, &semaphore);
            if (res != vk::Result::eSuccess)
            {
                VTNA_LOG_ERROR("[RHISwapchainVK] Failed to create AcquireSemaphore.");
                return false;
            }
            SetDebugName(device, vk::ObjectType::eSemaphore, (uint64_t)(VkSemaphore)semaphore, fmt::format("{} acquire semaphore {}", m_Name, i).c_str(), dynamicLoader);
            m_AcquireSemaphores.push_back(semaphore);
        }

        for (uint32_t i = 0; i < m_Desc.BufferCount; i++)
        {
            vk::Semaphore semaphore;

            vk::Result res = device.createSemaphore(&semaphoreCI, nullptr, &semaphore);
            if (res != vk::Result::eSuccess)
            {
                VTNA_LOG_ERROR("[RHISwapchainVK] Failed to create PresentSemaphore.");
                return false;
            }
            SetDebugName(device, vk::ObjectType::eSemaphore, (uint64_t)(VkSemaphore)semaphore, fmt::format("{} present semaphore {}", m_Name, i).c_str(), dynamicLoader);
            m_PresentSemaphores.push_back(semaphore);
        }
        return true;
    }

    bool RHISwapchainVK::RecreateSwapchain()
    {
        auto deviceVK = (RHIDeviceVK*)m_pDevice;
        auto device = deviceVK->GetDevice();
        device.waitIdle();

        for (size_t i = 0; i < m_BackBuffers.size(); i++)
        {
            delete m_BackBuffers[i];
        }
        m_BackBuffers.clear();

        for (size_t i = 0; i < m_AcquireSemaphores.size(); i++)
        {
            deviceVK->Delete(m_AcquireSemaphores[i]);
        }
        m_AcquireSemaphores.clear();

        for (size_t i = 0; i < m_PresentSemaphores.size(); i++)
        {
            deviceVK->Delete(m_PresentSemaphores[i]);
        }
        m_PresentSemaphores.clear();

        return CreateSwapchain() && CreateTextures() && CreateSemaphores();
    }
}