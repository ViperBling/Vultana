#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIHeap.hpp"

namespace RHI
{
    class FVulkanDevice;

    class FVulkanHeap : public FRHIHeap
    {
    public:
        FVulkanHeap(FVulkanDevice* device, const FRHIHeapDesc& desc, const eastl::string& name);
        ~FVulkanHeap();

        bool Create();

        virtual void* GetNativeHandle() const override { return m_Allocation; }

    private:
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
    };
}