#include "SwapchainVK.hpp"
#include "RHICommonVK.hpp"
#include "InstanceVK.hpp"
#include "DeviceVK.hpp"
#include "GPUVK.hpp"
#include "TextureVK.hpp"
#include "QueueVK.hpp"
#include "SurfaceVK.hpp"

namespace Vultana
{
    SwapchainVK::SwapchainVK(DeviceVK &device, const SwapchainCreateInfo &createInfo)
        : mDevice(device)
        , RHISwapchain(createInfo)
    {
        CreateNativeSwapchain(createInfo);
    }

    SwapchainVK::~SwapchainVK()
    {
        Destroy();
    }

    uint8_t SwapchainVK::AcquireBackTexture()
    {
        vk::resultCheck(mDevice.GetVkDevice().acquireNextImageKHR(mSwapchain, UINT64_MAX, mImageAvaliableSemaphore, nullptr, &mSwapchainImageIndex), "Device::AcquireNextImageKHR");
        return mSwapchainImageIndex;
    }

    void SwapchainVK::Present()
    {
        vk::PresentInfoKHR presentInfo {};
        presentInfo.setWaitSemaphores(mWaitSemaphores);
        presentInfo.setSwapchains(mSwapchain);
        presentInfo.setImageIndices(mSwapchainImageIndex);
        vk::resultCheck(mQueue.presentKHR(presentInfo), "Device::QueuePresentKHR");
        mWaitSemaphores.clear();
    }

    void SwapchainVK::Destroy()
    {
        auto device = mDevice.GetVkDevice();
        device.waitIdle();

        for (auto& tex : mTextures)
        {
            delete tex;
        }
        mTextures.clear();

        device.destroySemaphore(mImageAvaliableSemaphore);
        if (mSwapchain)
        {
            device.destroySwapchainKHR(mSwapchain);
        }
    }

    void SwapchainVK::CreateNativeSwapchain(const SwapchainCreateInfo &createInfo)
    {
        auto deviceVK = mDevice.GetVkDevice();
        auto* queue = dynamic_cast<QueueVK*>(createInfo.PresentQueue);
        assert(queue);
        auto* surface = dynamic_cast<SurfaceVK*>(createInfo.Surface);
        assert(surface);
        mQueue = queue->GetVkQueue();
        auto surfaceVK = surface->GetVkSurface();

        vk::SurfaceCapabilitiesKHR surfaceCaps = mDevice.GetGPU().GetVKPhysicalDevice().getSurfaceCapabilitiesKHR(surfaceVK);

        vk::Extent2D surfaceExtent = { static_cast<uint32_t>(createInfo.Extent.x), static_cast<uint32_t>(createInfo.Extent.y) };
        surfaceExtent = vk::Extent2D {
            std::clamp(surfaceExtent.width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width),
            std::clamp(surfaceExtent.height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height)
        };

        assert(mDevice.CheckSwapchainFormatSupport(surface, createInfo.Format));
        auto supportedFormat = VKEnumCast<RHIFormat, vk::Format>(createInfo.Format);
        auto colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

        std::vector<vk::PresentModeKHR> presentModes = mDevice.GetGPU().GetVKPhysicalDevice().getSurfacePresentModesKHR(surfaceVK);
        assert(!presentModes.empty());

        vk::PresentModeKHR supportedMode = VKEnumCast<RHIPresentMode, vk::PresentModeKHR>(createInfo.PresentMode);
        assert(!presentModes.empty());
        auto it = std::find_if(presentModes.begin(), presentModes.end(), [supportedMode](vk::PresentModeKHR mode) { return mode == supportedMode; });
        assert(it != presentModes.end());
        
        mSwapchainImageCount = std::clamp(static_cast<uint32_t>(createInfo.TextureCount), surfaceCaps.minImageCount, surfaceCaps.maxImageCount);
        vk::SwapchainCreateInfoKHR swapChainCI {};
        swapChainCI.setSurface(surfaceVK);
        swapChainCI.setMinImageCount(mSwapchainImageCount);
        swapChainCI.setImageFormat(supportedFormat);
        swapChainCI.setImageColorSpace(colorSpace);
        swapChainCI.setPresentMode(supportedMode);
        swapChainCI.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapChainCI.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapChainCI.setImageExtent(surfaceExtent);
        swapChainCI.setClipped(true);
        swapChainCI.setImageSharingMode(vk::SharingMode::eExclusive);
        swapChainCI.setImageArrayLayers(1);
        swapChainCI.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
        mSwapchain = deviceVK.createSwapchainKHR(swapChainCI);

        TextureCreateInfo textureCI {};
        textureCI.Format = createInfo.Format;
        textureCI.Usage = RHITextureUsageBits::CopyDst | RHITextureUsageBits::RenderAttachment;
        textureCI.MipLevels = 1;
        textureCI.Samples = 1;
        textureCI.Dimension = RHITextureDimension::Texture2D;
        textureCI.Extent.x = surfaceExtent.width;
        textureCI.Extent.y = surfaceExtent.height;
        textureCI.Extent.z = 1;

        vk::resultCheck(deviceVK.getSwapchainImagesKHR(mSwapchain, &mSwapchainImageCount, nullptr), "Get swapchain image");
        std::vector<vk::Image> swapchainImages(mSwapchainImageCount);
        swapchainImages = deviceVK.getSwapchainImagesKHR(mSwapchain /*, mDevice.GetGPU().GetInstance().GetVkDynamicLoader()*/);
        for (auto & image : swapchainImages)
        {
            mTextures.emplace_back(new TextureVK(mDevice, textureCI, image));
        }
        mSwapchainImageCount = static_cast<uint32_t>(swapchainImages.size());

        vk::SemaphoreCreateInfo semaphoreCI {};
        mImageAvaliableSemaphore = deviceVK.createSemaphore(semaphoreCI);
    }
} // namespace Vultana
