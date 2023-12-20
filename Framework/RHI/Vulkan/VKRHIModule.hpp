#pragma once

#include "RHI/RHIModule.hpp"

namespace Vultana
{
    RHIInstance* GetInstance();

    class VulkanRHIModule : public RHIModule
    {
    public:
        static VulkanRHIModule* GetModule();

        RHIInstance* GetRHIInstance() override
        {
            return GetInstance();
        }
    };
}