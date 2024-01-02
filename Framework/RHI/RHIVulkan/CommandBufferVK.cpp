#include "CommandBufferVK.hpp"

#include "DeviceVK.hpp"
#include "CommandListVK.hpp"

namespace RHI
{
    CommandBufferVK::CommandBufferVK(DeviceVK &device, vk::CommandPool commandPool)
        : mDevice(device)
        , mCommandPool(commandPool)
    {
        CreateNativeCommandBuffer();
    }

    CommandBufferVK::~CommandBufferVK()
    {
        Destroy();
    }

    void CommandBufferVK::Destroy()
    {
        auto deviceVK = mDevice.GetVkDevice();
        if (mCommandBuffer)
        {
            deviceVK.freeCommandBuffers(mCommandPool, mCommandBuffer);
            mCommandBuffer = nullptr;
        }
        for (auto& semaphore : mSignalSemaphores)
        {
            deviceVK.destroySemaphore(semaphore);
        }
    }

    RHICommandList *CommandBufferVK::Begin()
    {
        mWaitSemaphores.clear();
        mWaitStages.clear();

        vk::CommandBufferBeginInfo beginInfo {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        mCommandBuffer.begin(beginInfo);
        return new CommandListVK(mDevice, *this);
    }

    void CommandBufferVK::AddWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlags stage)
    {
        mWaitSemaphores.emplace_back(semaphore);
        mWaitStages.emplace_back(stage);
    }

    void CommandBufferVK::CreateNativeCommandBuffer()
    {
        vk::CommandBufferAllocateInfo commandBufferAI {};
        commandBufferAI.commandPool = mCommandPool;
        commandBufferAI.level = vk::CommandBufferLevel::ePrimary;
        commandBufferAI.commandBufferCount = 1;

        mCommandBuffer = mDevice.GetVkDevice().allocateCommandBuffers(commandBufferAI).front();

        vk::SemaphoreCreateInfo semaphoreCI {};
        mSignalSemaphores.resize(1);
        mSignalSemaphores[0] = mDevice.GetVkDevice().createSemaphore(semaphoreCI);
    }
}