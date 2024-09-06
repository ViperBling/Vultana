#include "RHISwapchainVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHITextureVK.hpp"
#include "RHI/RHICommon.hpp"
#include "Utilities/Log.hpp"
#include "Windows/GLFWindow.hpp"

namespace RHI
{
    RHISwapchainVK::RHISwapchainVK(RHIDeviceVK *device, const RHISwapchainDesc &desc, const std::string &name)
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
        if (!CreateSurface() || !CreateSwapchain() || !CreateTextures() || !CreateSemaphores())
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
        // TODO: Implement
    }

    bool RHISwapchainVK::CreateSurface()
    {
        vk::Instance instance = ((RHIDeviceVK*)mpDevice)->GetInstance();
        vk::Device device = ((RHIDeviceVK*)mpDevice)->GetDevice();
        
        // VkWin32SurfaceCreateInfoKHR surfaceCI {};
        auto wnd = (Window::GLFWindow*)mDesc.WindowHandle;
        vk::Result res = wnd->CreateVulkanSurface(instance, mSurface);
        if (res != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHISwapchainVK] Failed to create surface");
            return false;
        }
        SetDebugName(device, vk::ObjectType::eSurfaceKHR, (uint64_t)(VkSurfaceKHR)mSurface, mName.c_str());

        return true;
    }

    bool RHISwapchainVK::CreateSwapchain()
    {
        return false;
    }

    bool RHISwapchainVK::CreateTextures()
    {
        return false;
    }

    bool RHISwapchainVK::CreateSemaphores()
    {
        return false;
    }

    bool RHISwapchainVK::RecreateSwapchain()
    {
        return false;
    }
}