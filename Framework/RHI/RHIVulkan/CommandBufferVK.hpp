#pragma once

#include "RHI/RHICommandBuffer.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

namespace Vultana
{
    class DeviceVK;

    class CommandBufferVK : public RHICommandBuffer
    {
    public:
        NOCOPY(CommandBufferVK);
        CommandBufferVK(DeviceVK& device, vk::CommandPool commandPool);
        ~CommandBufferVK();
        void Destroy() override;

        RHICommandList* Begin() override;
        vk::CommandBuffer GetVkCommandBuffer() const { return mCommandBuffer; }

        void AddWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlags stage);
        const std::vector<vk::Semaphore>& GetWaitSemaphores() const { return mWaitSemaphores; }
        const std::vector<vk::Semaphore>& GetSignalSemaphores() const { return mSignalSemaphores; }
        const std::vector<vk::PipelineStageFlags>& GetWaitStages() const { return mWaitStages; }
        
    private:
        void CreateNativeCommandBuffer();

    private:
        DeviceVK& mDevice;
        vk::CommandPool mCommandPool;
        vk::CommandBuffer mCommandBuffer;
        std::vector<vk::Semaphore> mSignalSemaphores;
        std::vector<vk::Semaphore> mWaitSemaphores;
        std::vector<vk::PipelineStageFlags> mWaitStages;
    };
}