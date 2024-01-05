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

        RHIDevice* GetDevice() const { return mpDevice; }
        const std::string& GetName() const { return mName; }

    private:
        RHIDevice* mpDevice = nullptr;
        std::string mName;
    };
}