#include "RHIFenceVK.hpp"
#include "RHIDeviceVK.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHIFenceVK::RHIFenceVK(RHIDeviceVK *device, const std::string &name)
    {
        mpDevice = device;
        mName = name;
    }

    RHIFenceVK::~RHIFenceVK()
    {
        ((RHIDeviceVK*)mpDevice)->Delete(mSemaphore);
    }

    bool RHIFenceVK::Create()
    {
        auto device = ((RHIDeviceVK*)mpDevice)->GetDevice();
        auto dynamicLoader = ((RHIDeviceVK*)mpDevice)->GetDynamicLoader();

        vk::SemaphoreTypeCreateInfoKHR semaphoreTypeCI {};
        semaphoreTypeCI.semaphoreType = vk::SemaphoreType::eTimeline;
        semaphoreTypeCI.initialValue = 0;

        vk::SemaphoreCreateInfo semaphoreCI {};
        semaphoreCI.pNext = &semaphoreTypeCI;

        mSemaphore = device.createSemaphore(semaphoreCI);
        if (!mSemaphore)
        {
            VTNA_LOG_ERROR("[RHIFenceVK] Failed to create {}", mName);
            return false;
        }
        SetDebugName(device, vk::ObjectType::eSemaphore, mSemaphore, mName.c_str(), dynamicLoader);

        return true;
    }

    void RHIFenceVK::Wait(uint64_t value)
    {
        vk::SemaphoreWaitInfo waitInfo {};
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &mSemaphore;
        waitInfo.pValues = &value;
        
        auto device = ((RHIDeviceVK*)mpDevice)->GetDevice();
        assert(device.waitSemaphores(waitInfo, UINT64_MAX) == vk::Result::eSuccess);
    }

    void RHIFenceVK::Signal(uint64_t value)
    {
        vk::SemaphoreSignalInfo signalInfo {};
        signalInfo.semaphore = mSemaphore;
        signalInfo.value = value;

        auto device = ((RHIDeviceVK*)mpDevice)->GetDevice();
        device.signalSemaphore(signalInfo);
    }
}