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
        RHIDeviceVK* mDevice = nullptr;

        eastl::queue<eastl::pair<vk::Image, uint64_t>>          mImageQueue;
        eastl::queue<eastl::pair<vk::ImageView, uint64_t>>      mImageViewQueue;
        eastl::queue<eastl::pair<vk::Buffer, uint64_t>>         mBufferQueue;
        eastl::queue<eastl::pair<VmaAllocation, uint64_t>>      mAllocationQueue;
        eastl::queue<eastl::pair<vk::Sampler, uint64_t>>        mSamplerQueue;
        eastl::queue<eastl::pair<vk::Pipeline, uint64_t>>       mPipelineQueue;
        eastl::queue<eastl::pair<vk::ShaderModule, uint64_t>>   mShaderQueue;
        eastl::queue<eastl::pair<vk::Semaphore, uint64_t>>      mSemaphoreQueue;
        eastl::queue<eastl::pair<vk::SwapchainKHR, uint64_t>>   mSwapchainQueue;
        eastl::queue<eastl::pair<vk::SurfaceKHR, uint64_t>>     mSurfaceQueue;
        eastl::queue<eastl::pair<vk::CommandPool, uint64_t>>    mCommandPoolQueue;

        eastl::queue<eastl::pair<uint32_t, uint64_t>>           mResourceDescriptorQueue;
        eastl::queue<eastl::pair<uint32_t, uint64_t>>           mSamplerDescriptorQueue;
    };
}