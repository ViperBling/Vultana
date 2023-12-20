#pragma once

#include "RHICommon.hpp"
#include "RHIModule.hpp"

namespace Vultana
{
    class RHIInstance
    {
    public:
        static RHIInstance* GetByRHIBackend(const RHIRenderBackend& backend)
        {
            RHIInstance* instance = nullptr;

            switch (backend)
            {
            case RHIRenderBackend::Vulkan:
                rhiModule = RHIModule::GetRHIInstance();
                break;
            
            default:
                break;
            }
            return instance;
        }

        NOCOPY(RHIInstance)
        virtual ~RHIInstance() = default;

        virtual RHIRenderBackend GetRHIBackend() = 0;
        virtual void Destroy() = 0;

    protected:
        explicit RHIInstance() = default;
    };
}