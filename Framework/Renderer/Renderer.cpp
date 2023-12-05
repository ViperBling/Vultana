#include "Renderer.hpp"
#include "Windows/GLFWindow.hpp"

#include <optional>

using namespace Vultana;

namespace Vultana::Renderer
{
    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    const bool enableValidationLayers = true;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    }; 


    RendererBase::~RendererBase()
    {
        CleanupSwapchain();

        mDevice.destroyPipeline(mGraphicsPipeline);
        mDevice.destroyPipelineLayout(mPipelineLayout);
        mDevice.destroyRenderPass(mRenderPass);

        for (size_t i = 0; i < 3; i++)
        {
            mDevice.destroySemaphore(mRenderFinishedSemaphores[i]);
            mDevice.destroySemaphore(mImageAvailableSemaphores[i]);
            mDevice.destroyFence(mInFlightFences[i]);
        }

        mDevice.destroyCommandPool(mCommandPool);
        mDevice.destroy();

        mInstance.destroySurfaceKHR(mSurface);
        mInstance.destroy();
    }

    void RendererBase::Init()
    {
        InitVulkan();
        CreateInstance();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapchain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
    }

    void RendererBase::ConnectSurface(void *windowHandle)
    {
        
    }

    void RendererBase::RenderFrame()
    {
    }

    void RendererBase::InitVulkan()
    {

    }

    void RendererBase::CreateInstance()
    {
        vk::ApplicationInfo appInfo = {};
        appInfo.pApplicationName = "Vultana";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Vultana";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        vk::InstanceCreateInfo instanceCI {};
        instanceCI.pApplicationInfo = &appInfo;

        if (enableValidationLayers && !CheckValidationLayerSupport())
        {
            throw std::runtime_error("Validation layers requested, but not available!");
        }
        else
        {
            instanceCI.enabledLayerCount = 0;
        }

        mInstance = vk::createInstance(instanceCI);
    }

    void RendererBase::PickPhysicalDevice()
    {
    }

    void RendererBase::CreateLogicalDevice()
    {
    }

    void RendererBase::CreateSwapchain()
    {
    }

    void RendererBase::CreateImageViews()
    {
    }

    void RendererBase::CreateRenderPass()
    {
    }

    void RendererBase::CreateGraphicsPipeline()
    {
    }

    void RendererBase::CreateFramebuffers()
    {
    }

    void RendererBase::CreateCommandPool()
    {
    }

    void RendererBase::CreateCommandBuffers()
    {
    }

    void RendererBase::CreateSyncObjects()
    {
    }

    void RendererBase::CleanupSwapchain()
    {
    }

    bool RendererBase::CheckValidationLayerSupport()
    {
        auto availableLayers = vk::enumerateInstanceLayerProperties();

        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }
        return true;
    }

} // namespace Vultana::Renderer
