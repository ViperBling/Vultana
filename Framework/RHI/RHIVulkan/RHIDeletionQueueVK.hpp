#pragma once

#include "RHICommonVK.hpp"

#include <EASTL/queue.h>

namespace RHI
{
    class RHIDeviceVK;

    class RHIDeletionQueueVK
    {
    public:
        RHIDeletionQueueVK(RHIDeviceVK* device);
        ~RHIDeletionQueueVK();

        void Flush(bool forceDelete = false);

        template<typename T>
        void Delete(T object, uint64_t frameID);

        void FreeResourceDescriptor(uint32_t index, uint64_t frameID);
        void FreeSamplerDescriptor(uint32_t index, uint64_t frameID);
    
    private:
        RHIDeviceVK* m_Device = nullptr;

        eastl::queue<eastl::pair<vk::Image, uint64_t>>          m_ImageQueue;
        eastl::queue<eastl::pair<vk::ImageView, uint64_t>>      m_ImageViewQueue;
        eastl::queue<eastl::pair<vk::Buffer, uint64_t>>         m_BufferQueue;
        eastl::queue<eastl::pair<VmaAllocation, uint64_t>>      m_AllocationQueue;
        eastl::queue<eastl::pair<vk::Sampler, uint64_t>>        m_SamplerQueue;
        eastl::queue<eastl::pair<vk::Pipeline, uint64_t>>       m_PipelineQueue;
        eastl::queue<eastl::pair<vk::ShaderModule, uint64_t>>   m_ShaderQueue;
        eastl::queue<eastl::pair<vk::Semaphore, uint64_t>>      m_SemaphoreQueue;
        eastl::queue<eastl::pair<vk::SwapchainKHR, uint64_t>>   m_SwapchainQueue;
        eastl::queue<eastl::pair<vk::SurfaceKHR, uint64_t>>     m_SurfaceQueue;
        eastl::queue<eastl::pair<vk::CommandPool, uint64_t>>    m_CommandPoolQueue;

        eastl::queue<eastl::pair<uint32_t, uint64_t>>           m_ResourceDescriptorQueue;
        eastl::queue<eastl::pair<uint32_t, uint64_t>>           m_SamplerDescriptorQueue;
    };
}