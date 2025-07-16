#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    class RHIDevice;

    class RHIResource
    {
    public:
        virtual ~RHIResource() = default;

        virtual void* GetNativeHandle() const = 0;
        virtual bool IsTexture() const { return false; }
        virtual bool IsBuffer() const { return false; }

        RHIDevice* GetDevice() const { return m_pDevice; }
        const eastl::string& GetName() const { return m_Name; }

    protected:
        RHIDevice* m_pDevice = nullptr;
        eastl::string m_Name;
    };
}