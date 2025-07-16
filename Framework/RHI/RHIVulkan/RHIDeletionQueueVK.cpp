#include "RHIDeletionQueueVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHIDescriptorAllocatorVK.hpp"

namespace RHI
{
    RHIDeletionQueueVK::RHIDeletionQueueVK(RHIDeviceVK *device)
    {
        m_Device = device;
    }

    RHIDeletionQueueVK::~RHIDeletionQueueVK()
    {
        Flush(true);
    }

    void RHIDeletionQueueVK::Flush(bool forceDelete)
    {
        uint64_t frameID = m_Device->GetFrameID();
        vk::Instance instance = m_Device->GetInstance();
        vk::Device device = m_Device->GetDevice();
        VmaAllocator allocator = m_Device->GetVmaAllocator();

        #define ITERATE_QUEUE(queue, deletionFunc)                                   \
            while (!queue.empty())                                                   \
            {                                                                        \
                auto item = queue.front();                                           \
                if (!forceDelete && item.second + RHI_MAX_INFLIGHT_FRAMES > frameID) \
                {                                                                    \
                    break;                                                           \
                }                                                                    \
                deletionFunc(item.first);                                            \
                queue.pop();                                                         \
            }
        
        ITERATE_QUEUE(m_ImageQueue, device.destroyImage)
        ITERATE_QUEUE(m_ImageViewQueue, device.destroyImageView)
        ITERATE_QUEUE(m_BufferQueue, device.destroyBuffer)
        ITERATE_QUEUE(m_SamplerQueue, device.destroySampler)
        ITERATE_QUEUE(m_PipelineQueue, device.destroyPipeline)
        ITERATE_QUEUE(m_ShaderQueue, device.destroyShaderModule)
        ITERATE_QUEUE(m_SemaphoreQueue, device.destroySemaphore)
        ITERATE_QUEUE(m_SwapchainQueue, device.destroySwapchainKHR)
        ITERATE_QUEUE(m_CommandPoolQueue, device.destroyCommandPool)

        while (!m_SurfaceQueue.empty())
        {
            auto item = m_SurfaceQueue.front();
            if (!forceDelete && item.second + RHI_MAX_INFLIGHT_FRAMES > frameID)
            {
                break;
            }
            instance.destroySurfaceKHR(item.first);
            m_SurfaceQueue.pop();
        }
        while (!m_AllocationQueue.empty())
        {
            auto item = m_AllocationQueue.front();
            if (!forceDelete && item.second + RHI_MAX_INFLIGHT_FRAMES > frameID)
            {
                break;
            }
            vmaFreeMemory(allocator, item.first);
            m_AllocationQueue.pop();
        }
        while (!m_ResourceDescriptorQueue.empty())
        {
            auto item = m_ResourceDescriptorQueue.front();
            if (!forceDelete && item.second + RHI_MAX_INFLIGHT_FRAMES > frameID)
            {
                break;
            }
            m_Device->GetResourceDescriptorAllocator()->Free(item.first);
            m_ResourceDescriptorQueue.pop();
        }
        while (!m_SamplerDescriptorQueue.empty())
        {
            auto item = m_SamplerDescriptorQueue.front();
            if (!forceDelete && item.second + RHI_MAX_INFLIGHT_FRAMES > frameID)
            {
                break;
            }
            m_Device->GetSamplerDescriptorAllocator()->Free(item.first);
            m_SamplerDescriptorQueue.pop();
        }
    }

    void RHIDeletionQueueVK::FreeResourceDescriptor(uint32_t index, uint64_t frameID)
    {
        m_ResourceDescriptorQueue.push(eastl::make_pair(index, frameID));
    }

    void RHIDeletionQueueVK::FreeSamplerDescriptor(uint32_t index, uint64_t frameID)
    {
        m_SamplerDescriptorQueue.push(eastl::make_pair(index, frameID));
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::Image object, uint64_t frameID)
    {
        m_ImageQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::ImageView object, uint64_t frameID)
    {
        m_ImageViewQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::Buffer object, uint64_t frameID)
    {
        m_BufferQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(VmaAllocation object, uint64_t frameID)
    {
        m_AllocationQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::Sampler object, uint64_t frameID)
    {
        m_SamplerQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::Pipeline object, uint64_t frameID)
    {
        m_PipelineQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::ShaderModule object, uint64_t frameID)
    {
        m_ShaderQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::Semaphore object, uint64_t frameID)
    {
        m_SemaphoreQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::SwapchainKHR object, uint64_t frameID)
    {
        m_SwapchainQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::SurfaceKHR object, uint64_t frameID)
    {
        m_SurfaceQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::CommandPool object, uint64_t frameID)
    {
        m_CommandPoolQueue.push({ object, frameID });
    }
}