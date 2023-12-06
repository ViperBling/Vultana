#pragma once

#include <string>

#include "RHICommon.hpp"

namespace Vultana
{
    class RHIDevice;

    class RHIResource
    {
    public:
        virtual ~RHIResource() {};

        virtual void* GetHandle() const = 0;
        virtual bool IsTexture() const { return false; }
        virtual bool IsBuffer() const { return false; }

        const std::string& GetName() const { return mName; }
        RHIDevice* GetDevice() const { return mpDevice; }

    protected:
        RHIDevice* mpDevice = nullptr;
        std::string mName;
    };
}