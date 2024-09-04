#include "RHIDeletionQueueVK.hpp"
#include "RHIDeviceVK.hpp"

namespace RHI
{
    RHIDeletionQueueVK::RHIDeletionQueueVK(RHIDeviceVK *device)
    {
        mDevice = device;
    }

    RHIDeletionQueueVK::~RHIDeletionQueueVK()
    {
        Flush(true);
    }

    void RHIDeletionQueueVK::Flush(bool forceDelete)
    {
        uint64_t frameID = mDevice->GetFrameID();
        vk::Instance instance = mDevice->GetInstance();
        vk::Device device = mDevice->GetDevice();
        VmaAllocator allocator = mDevice->GetVmaAllocator();

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
        
        ITERATE_QUEUE(mImageQueue, device.destroyImage)
        ITERATE_QUEUE(mImageViewQueue, device.destroyImageView)
        ITERATE_QUEUE(mBufferQueue, device.destroyBuffer)
        ITERATE_QUEUE(mSamplerQueue, device.destroySampler)
        ITERATE_QUEUE(mPipelineQueue, device.destroyPipeline)
        ITERATE_QUEUE(mShaderQueue, device.destroyShaderModule)
        ITERATE_QUEUE(mSemaphoreQueue, device.destroySemaphore)
        ITERATE_QUEUE(mSwapchainQueue, device.destroySwapchainKHR)
        ITERATE_QUEUE(mCommandPoolQueue, device.destroyCommandPool)

        while (!mSurfaceQueue.empty())
        {
            auto item = mSurfaceQueue.front();
            if (!forceDelete && item.second + RHI_MAX_INFLIGHT_FRAMES > frameID)
            {
                break;
            }
            instance.destroySurfaceKHR(item.first);
            mSurfaceQueue.pop();
        }
        while (!mAllocationQueue.empty())
        {
            auto item = mAllocationQueue.front();
            if (!forceDelete && item.second + RHI_MAX_INFLIGHT_FRAMES > frameID)
            {
                break;
            }
            vmaFreeMemory(allocator, item.first);
            mAllocationQueue.pop();
        }
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::Image object, uint64_t frameID)
    {
        mImageQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::ImageView object, uint64_t frameID)
    {
        mImageViewQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::Buffer object, uint64_t frameID)
    {
        mBufferQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(VmaAllocation object, uint64_t frameID)
    {
        mAllocationQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::Sampler object, uint64_t frameID)
    {
        mSamplerQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::Pipeline object, uint64_t frameID)
    {
        mPipelineQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::ShaderModule object, uint64_t frameID)
    {
        mShaderQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::Semaphore object, uint64_t frameID)
    {
        mSemaphoreQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::SwapchainKHR object, uint64_t frameID)
    {
        mSwapchainQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::SurfaceKHR object, uint64_t frameID)
    {
        mSurfaceQueue.push({ object, frameID });
    }

    template<>
    void RHIDeletionQueueVK::Delete(vk::CommandPool object, uint64_t frameID)
    {
        mCommandPoolQueue.push({ object, frameID });
    }
}