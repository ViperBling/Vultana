#pragma once

#include "RHICommonVK.hpp"
#include <queue>

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
    
    private:
        RHIDeviceVK* mDevice = nullptr;

        std::queue<std::pair<vk::Image, uint64_t>>          mImageQueue;
        std::queue<std::pair<vk::ImageView, uint64_t>>      mImageViewQueue;
        std::queue<std::pair<vk::Buffer, uint64_t>>         mBufferQueue;
        std::queue<std::pair<VmaAllocation, uint64_t>>      mAllocationQueue;
        std::queue<std::pair<vk::Sampler, uint64_t>>        mSamplerQueue;
        std::queue<std::pair<vk::Pipeline, uint64_t>>       mPipelineQueue;
        std::queue<std::pair<vk::ShaderModule, uint64_t>>   mShaderQueue;
        std::queue<std::pair<vk::Semaphore, uint64_t>>      mSemaphoreQueue;
        std::queue<std::pair<vk::SwapchainKHR, uint64_t>>   mSwapchainQueue;
        std::queue<std::pair<vk::SurfaceKHR, uint64_t>>     mSurfaceQueue;
        std::queue<std::pair<vk::CommandPool, uint64_t>>    mCommandPoolQueue;
    };
}