#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    class FRHIDevice;

    class FRHIResource
    {
    public:
        virtual ~FRHIResource() = default;

        virtual void* GetNativeHandle() const = 0;
        virtual bool IsTexture() const { return false; }
        virtual bool IsBuffer() const { return false; }

        FRHIDevice* GetDevice() const { return m_pDevice; }
        const eastl::string& GetName() const { return m_Name; }

    protected:
        FRHIDevice* m_pDevice = nullptr;
        eastl::string m_Name;
    };
}