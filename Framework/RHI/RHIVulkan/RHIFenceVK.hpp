#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHIFence.hpp"

namespace RHI
{
    class FVulkanDevice;

    class FVulkanFence : public FRHIFence
    {
    public:
        FVulkanFence(FVulkanDevice* device, const eastl::string& name);
        ~FVulkanFence();

        bool Create();

        virtual void* GetNativeHandle() const override { return m_Semaphore; }
        virtual void Wait(uint64_t value) override;
        virtual void Signal(uint64_t value) override;

    private:
        vk::Semaphore m_Semaphore;
    };
}