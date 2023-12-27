#include "SynchronousVK.hpp"

#include "RHICommonVK.hpp"
#include "DeviceVK.hpp"

namespace Vultana
{
    FenceVK::FenceVK(DeviceVK &device)
        : RHIFence(device)
        , mDevice(device)
        , mbSignaled(false)
    {
        CreateFence();
    }

    FenceVK::~FenceVK()
    {
        Destroy();
    }
    
    FenceStatus FenceVK::GetFenceStatus()
    {
        return mbSignaled ? FenceStatus::Signal : FenceStatus::Wait;
    }

    void FenceVK::Reset()
    {
        mDevice.GetVkDevice().resetFences(mFence);
        mbSignaled = false;
    }

    void FenceVK::Wait()
    {
        if (mbSignaled) { return; }
        vk::resultCheck(mDevice.GetVkDevice().waitForFences(mFence, true, UINT64_MAX), "Fence: Wait");
    }

    void FenceVK::Destroy()
    {
        if (mFence)
        {
            mDevice.GetVkDevice().destroyFence(mFence);
            mFence = VK_NULL_HANDLE;
        }
    }

    void FenceVK::CreateFence()
    {
        vk::FenceCreateInfo fenceCI {};

        mFence = mDevice.GetVkDevice().createFence(fenceCI);
    }
}