#pragma once

#include "RHI/RHI.hpp"

#include <EASTL/unique_ptr.h>

namespace Renderer
{
    class RendererBase;
}

namespace RenderResources
{
    class Texture2D
    {
    public:
        Texture2D(const eastl::string& name);

        bool Create(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags);

        RHI::RHITexture* GetTexture() const { return mpTexture.get(); }
        RHI::RHIDescriptor* GetSRV() const { return mpSRV.get(); }
        RHI::RHIDescriptor* GetUAV(uint32_t mip = 0) const;
    
    protected:
        eastl::string mName;

        eastl::unique_ptr<RHI::RHITexture> mpTexture;
        eastl::unique_ptr<RHI::RHIDescriptor> mpSRV;
        eastl::vector<eastl::unique_ptr<RHI::RHIDescriptor>> mUAVs;
    };
}