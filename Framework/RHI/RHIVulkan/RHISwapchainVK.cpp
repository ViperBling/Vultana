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
        mpDevice = device;
        mDesc = desc;
        mName = name;
    }

    RHISwapchainVK::~RHISwapchainVK()
    {
        for (size_t i = 0; i < mBackBuffers.size(); i++)
        {
            delete mBackBuffers[i];
        }
        mBackBuffers.clear();

        auto device = (RHIDeviceVK*)mpDevice;
        device->Delete(mSwapchain);
        device->Delete(mSurface);

        for (size_t i = 0; i < mAcquireSemaphores.size(); i++)
        {
            device->Delete(mAcquireSemaphores[i]);
        }
        for (size_t i = 0; i < mPresentSemaphores.size(); i++)
        {
            device->Delete(mPresentSemaphores[i]);
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
            VTNA_LOG_ERROR("[RHISwapchainVK] Failed to create {}", mName);
            return false;
        }
        return true;
    }

    void RHISwapchainVK::Present(vk::Queue queue)
    {
        vk::Semaphore waitSemphore = GetPresentSemaphore();

        vk::PresentInfoKHR presentInfo {};
        presentInfo.setWaitSemaphores(waitSemphore);
        presentInfo.setSwapchains(mSwapchain);
        presentInfo.setImageIndices(mCurrentBackBuffer);

        vk::Result res = queue.presentKHR(presentInfo);

        if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapchain();
        }
    }

    vk::Semaphore RHISwapchainVK::GetAcquireSemaphore()
    {
        return mAcquireSemaphores[mFrameSemaphoreIndex];
    }

    vk::Semaphore RHISwapchainVK::GetPresentSemaphore()
    {
        return mPresentSemaphores[mFrameSemaphoreIndex];
    }

    void RHISwapchainVK::AcquireNextBackBuffer()
    {
        mFrameSemaphoreIndex = (mFrameSemaphoreIndex + 1) % mAcquireSemaphores.size();

        vk::Semaphore signalSemphore = GetAcquireSemaphore();

        vk::Result res = ((RHIDeviceVK*)mpDevice)->GetDevice().acquireNextImageKHR(mSwapchain, UINT64_MAX, signalSemphore, nullptr, &mCurrentBackBuffer);

        if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapchain();

            res = ((RHIDeviceVK*)mpDevice)->GetDevice().acquireNextImageKHR(mSwapchain, UINT64_MAX, signalSemphore, nullptr, &mCurrentBackBuffer);
            assert(res == vk::Result::eSuccess);
        }
    }

    RHITexture *RHISwapchainVK::GetBackBuffer() const
    {
        return mBackBuffers[mCurrentBackBuffer];
    }

    bool RHISwapchainVK::Resize(uint32_t width, uint32_t height)
    {
        if (mDesc.Width == width && mDesc.Height == height)
        {
            return false;
        }
        mDesc.Width = width;
        mDesc.Height = height;
        return RecreateSwapchain();
    }

    void RHISwapchainVK::SetVSyncEnabled(bool enabled)
    {
        if (mbEnableVSync != enabled)
        {
            mbEnableVSync = enabled;
            RecreateSwapchain();
        }
    }

    bool RHISwapchainVK::CreateSurface()
    {
        vk::Instance instance = ((RHIDeviceVK*)mpDevice)->GetInstance();
        vk::Device device = ((RHIDeviceVK*)mpDevice)->GetDevice();
        vk::DispatchLoaderDynamic dynamicLoader = ((RHIDeviceVK*)mpDevice)->GetDynamicLoader();
        vk::PhysicalDevice physcialDevice = ((RHIDeviceVK*)mpDevice)->GetPhysicalDevice();
        
        vk::Win32SurfaceCreateInfoKHR surfaceCI {};
        surfaceCI.hinstance = GetModuleHandle(nullptr);
        surfaceCI.hwnd = (HWND)mDesc.WindowHandle;
        // auto wnd = (Window::GLFWindow*)mDesc.WindowHandle;
        // vk::Result res = wnd->CreateVulkanSurface(instance, mSurface);
        mSurface = instance.createWin32SurfaceKHR(surfaceCI, nullptr);

        SetDebugName(device, vk::ObjectType::eSurfaceKHR, (uint64_t)(VkSurfaceKHR)mSurface, mName.c_str(), dynamicLoader);

        auto presentMode = physcialDevice.getSurfacePresentModesKHR(mSurface);
        mbMailboxSupported = eastl::find(presentMode.begin(), presentMode.end(), vk::PresentModeKHR::eMailbox) != presentMode.end();

        return true;
    }

    bool RHISwapchainVK::CreateSwapchain()
    {
        vk::Device device = ((RHIDeviceVK*)mpDevice)->GetDevice();
        auto dynamicLoader = ((RHIDeviceVK*)mpDevice)->GetDynamicLoader();
        auto physcialDevice = ((RHIDeviceVK*)mpDevice)->GetPhysicalDevice();
        vk::SwapchainKHR oldSwapchain = mSwapchain;

        auto presentMode = physcialDevice.getSurfacePresentModesKHR(mSurface);
        auto surfaceCaps = physcialDevice.getSurfaceCapabilitiesKHR(mSurface);

        // vk::Extent2D extent = vk::Extent2D(
        //     eastl::clamp(mDesc.Width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width),
        //     eastl::clamp(mDesc.Height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height)
        // );
        vk::Extent2D extent = vk::Extent2D(mDesc.Width, mDesc.Height);

        vk::Format viewFormats[2];
        viewFormats[0] = ToVulkanFormat(mDesc.ColorFormat);
        viewFormats[1] = ToVulkanFormat(mDesc.ColorFormat, true);

        vk::ImageFormatListCreateInfo imageFormatListCI {};
        imageFormatListCI.setViewFormats(viewFormats);

        vk::SwapchainCreateInfoKHR swapchainCI {};
        swapchainCI.setSurface(mSurface);
        swapchainCI.setMinImageCount(mDesc.BufferCount);
        swapchainCI.setImageFormat(ToVulkanFormat(mDesc.ColorFormat));
        swapchainCI.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
        swapchainCI.setImageExtent(extent);
        swapchainCI.setImageArrayLayers(1);
        swapchainCI.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        swapchainCI.setImageSharingMode(vk::SharingMode::eExclusive);
        swapchainCI.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapchainCI.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapchainCI.setPresentMode(mbEnableVSync ? vk::PresentModeKHR::eFifo : (mbMailboxSupported ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eImmediate));
        swapchainCI.setClipped(true);
        swapchainCI.setOldSwapchain(oldSwapchain);

        if (IsSRGBFormat(mDesc.ColorFormat))
        {
            swapchainCI.setFlags(vk::SwapchainCreateFlagBitsKHR::eMutableFormat);
            swapchainCI.setPNext(&imageFormatListCI);
        }

        vk::Result res = device.createSwapchainKHR(&swapchainCI, nullptr, &mSwapchain);
        if (res != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHISwapchainVK] Failed to create swapchain");
            return false;
        }
        SetDebugName(device, vk::ObjectType::eSwapchainKHR, (uint64_t)(VkSwapchainKHR)mSwapchain, mName.c_str(), dynamicLoader);

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            ((RHIDeviceVK*)mpDevice)->Delete(oldSwapchain);
        }
        return true;
    }

    bool RHISwapchainVK::CreateTextures()
    {
        vk::Device device = (VkDevice)mpDevice->GetNativeHandle();

        RHI::RHITextureDesc desc {};
        desc.Width = mDesc.Width;
        desc.Height = mDesc.Height;
        desc.Format = mDesc.ColorFormat;
        desc.Usage = RHITextureUsageRenderTarget;

        auto images = device.getSwapchainImagesKHR(mSwapchain);

        for (uint32_t i = 0; i < images.size(); i++)
        {
            eastl::string name = fmt::format("{} texture {}", mName, i).c_str();

            RHITextureVK* texture = new RHITextureVK((RHIDeviceVK*)mpDevice, desc, name);
            texture->Create(images[i]);
            
            mBackBuffers.push_back(texture);
        }
        return true;
    }

    bool RHISwapchainVK::CreateSemaphores()
    {
        vk::Device device = ((RHIDeviceVK*)mpDevice)->GetDevice();
        auto dynamicLoader = ((RHIDeviceVK*)mpDevice)->GetDynamicLoader();
        vk::SemaphoreCreateInfo semaphoreCI {};

        for (uint32_t i = 0; i < mDesc.BufferCount; i++)
        {
            vk::Semaphore semaphore;

            vk::Result res = device.createSemaphore(&semaphoreCI, nullptr, &semaphore);
            if (res != vk::Result::eSuccess)
            {
                VTNA_LOG_ERROR("[RHISwapchainVK] Failed to create AcquireSemaphore.");
                return false;
            }
            SetDebugName(device, vk::ObjectType::eSemaphore, (uint64_t)(VkSemaphore)semaphore, fmt::format("{} acquire semaphore {}", mName, i).c_str(), dynamicLoader);
            mAcquireSemaphores.push_back(semaphore);
        }

        for (uint32_t i = 0; i < mDesc.BufferCount; i++)
        {
            vk::Semaphore semaphore;

            vk::Result res = device.createSemaphore(&semaphoreCI, nullptr, &semaphore);
            if (res != vk::Result::eSuccess)
            {
                VTNA_LOG_ERROR("[RHISwapchainVK] Failed to create PresentSemaphore.");
                return false;
            }
            SetDebugName(device, vk::ObjectType::eSemaphore, (uint64_t)(VkSemaphore)semaphore, fmt::format("{} present semaphore {}", mName, i).c_str(), dynamicLoader);
            mPresentSemaphores.push_back(semaphore);
        }
        return true;
    }

    bool RHISwapchainVK::RecreateSwapchain()
    {
        auto deviceVK = (RHIDeviceVK*)mpDevice;
        auto device = deviceVK->GetDevice();
        device.waitIdle();

        for (size_t i = 0; i < mBackBuffers.size(); i++)
        {
            delete mBackBuffers[i];
        }
        mBackBuffers.clear();

        for (size_t i = 0; i < mAcquireSemaphores.size(); i++)
        {
            deviceVK->Delete(mAcquireSemaphores[i]);
        }
        mAcquireSemaphores.clear();

        for (size_t i = 0; i < mPresentSemaphores.size(); i++)
        {
            deviceVK->Delete(mPresentSemaphores[i]);
        }
        mPresentSemaphores.clear();

        return CreateSwapchain() && CreateTextures() && CreateSemaphores();
    }
}