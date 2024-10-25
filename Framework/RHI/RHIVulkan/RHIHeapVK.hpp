#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIHeap.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHIHeapVK : public RHIHeap
    {
    public:
        RHIHeapVK(RHIDeviceVK* device, const RHIHeapDesc& desc, const eastl::string& name);
        ~RHIHeapVK();

        bool Create();

        virtual void* GetNativeHandle() const override { return mAllocation; }

    private:
        VmaAllocation mAllocation = VK_NULL_HANDLE;
    };
}