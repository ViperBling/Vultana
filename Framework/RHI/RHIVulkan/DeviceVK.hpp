#pragma once

#include "RHI/RHIDevice.hpp"
#include "Utilities/Utility.hpp"

#include <optional>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class GPUVK;
    class QueueVK;

    class DeviceVK : public RHIDevice
    {
    public:
        NOCOPY(DeviceVK)
        DeviceVK(GPUVK& gpu, const DeviceCreateInfo& createInfo);
        ~DeviceVK();
        void Destroy() override;

        size_t GetQueueCount(RHICommandQueueType type) override;
        RHIQueue* GetQueue(RHICommandQueueType type, size_t index) override;
        RHISurface* CreateSurface(const SurfaceCreateInfo& createInfo) override;
        RHISwapchain* CreateSwapchain(const SwapchainCreateInfo& createInfo) override;
        RHIBuffer* CreateBuffer(const BufferCreateInfo& createInfo) override;
        RHITexture* CreateTexture(const TextureCreateInfo& createInfo) override;
        RHISampler* CreateSampler(const SamplerCreateInfo& createInfo) override;
        RHIBindGroup* CreateBindGroup(const BindGroupCreateInfo& createInfo) override;
        RHIBindGroupLayout* CreateBindGroupLayout(const BindGroupLayoutCreateInfo& createInfo) override;
        RHIPipelineLayout* CreatePipelineLayout(const PipelineLayoutCreateInfo& createInfo) override;
        RHIShaderModule* CreateShaderModule(const ShaderModuleCreateInfo& createInfo) override;
        RHIGraphicsPipeline* CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;
        RHIComputePipeline* CreateComputePipeline(const ComputePipelineCreateInfo& createInfo) override;
        RHICommandBuffer* CreateCommandBuffer() override;
        RHIFence* CreateFence(bool signaled) override;

        bool CheckSwapchainFormatSupport(RHISurface* surface, RHIFormat format) override;

        vk::Device GetVkDevice() const { return mDevice; }
        GPUVK& GetGPU() const { return mGPU; }
        VmaAllocator& GetVkAllocator() { return mAllocator; }

        void SetObjectName(vk::ObjectType objectType, uint64_t handle, const std::string& name);

    private:
        static std::optional<uint32_t> FindQueueFamilyIndex(const std::vector<vk::QueueFamilyProperties>& properties, std::vector<uint32_t> usedQueueFamily, RHICommandQueueType type);
        void CreateDevice(const DeviceCreateInfo& createInfo);
        void GetQueues();
        void CreateVmaAllocator();

    private:
        GPUVK& mGPU;
        vk::Device mDevice;
        VmaAllocator mAllocator;

        std::unordered_map<RHICommandQueueType, std::pair<uint32_t, uint32_t>> mQueueFamilyMappings;
        std::unordered_map<RHICommandQueueType, std::vector<std::unique_ptr<QueueVK>>> mQueues;
        std::unordered_map<RHICommandQueueType, vk::CommandPool> mCommandPools;
    };
} // namespace Vultana::RHI
