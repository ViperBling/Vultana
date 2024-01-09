#include "RHIDeviceVK.hpp"

#include <map>

namespace RHI::Vulkan
{
     const std::vector<const char *> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        // VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
    };

    const std::vector<const char *> DEVICE_LAYERS = {
        VK_KRONOS_VALIDATION_LAYER_NAME,
    };

    RHIDeviceVK::~RHIDeviceVK()
    {
        vmaDestroyAllocator(mAllocator);
        mDevice.destroy();
    }

    bool RHIDeviceVK::Initialize()
    {
        auto queueFamilyProps = mInstance.GetPhysicalDevice().getQueueFamilyProperties();

        std::map<ERHICommandQueueType, uint32_t> queueNumMap;
        return true;
    }

    void RHIDeviceVK::BeginFrame()
    {
    }

    void RHIDeviceVK::EndFrame()
    {
    }

    uint64_t RHIDeviceVK::GetFrameID() const
    {
        return 0;
    }

    RHISwapchain *RHIDeviceVK::CreateSwapchain(const RHISwapchainDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHICommandList *RHIDeviceVK::CreateCommandList(ERHICommandQueueType queueType, const std::string &name)
    {
        return nullptr;
    }

    RHIFence *RHIDeviceVK::CreateFence(const std::string &name)
    {
        return nullptr;
    }

    RHIHeap *RHIDeviceVK::CreateHeap(const RHIHeapDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIBuffer *RHIDeviceVK::CreateBuffer(const RHIBufferDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHITexture *RHIDeviceVK::CreateTexture(const RHITextureDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIShader *RHIDeviceVK::CreateShader(const RHIShaderDesc &desc, std::span<uint8_t> data, const std::string &name)
    {
        return nullptr;
    }

    RHIPipelineState *RHIDeviceVK::CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIPipelineState *RHIDeviceVK::CreateComputePipelineState(const RHIComputePipelineStateDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIDescriptor *RHIDeviceVK::CreateShaderResourceView(RHIResource *resource, const RHIShaderResourceViewDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIDescriptor *RHIDeviceVK::CreateUnorderedAccessView(RHIResource *resource, const RHIUnorderedAccessViewDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIDescriptor *RHIDeviceVK::CreateConstantBufferView(RHIResource *resource, const RHIConstantBufferViewDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    RHIDescriptor *RHIDeviceVK::CreateSampler(const RHISamplerDesc &desc, const std::string &name)
    {
        return nullptr;
    }

    uint32_t RHIDeviceVK::GetAllocationSize(const RHIBufferDesc &desc) const
    {
        return 0;
    }

    uint32_t RHIDeviceVK::GetAllocationSize(const RHITextureDesc &desc) const
    {
        return 0;
    }
}