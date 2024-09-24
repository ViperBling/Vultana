#pragma once

#include "RHI/RHI.hpp"

#include <memory>

namespace Renderer
{
    class RendererBase;

    class Texture2D
    {
    public:
        Texture2D(const std::string& name);

        bool Create(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags);

        RHI::RHITexture* GetTexture() const { return mpTexture.get(); }
        RHI::RHIDescriptor* GetSRV() const { return mpSRV.get(); }
        RHI::RHIDescriptor* GetUAV(uint32_t mip = 0) const;
    
    protected:
        std::string mName;

        std::unique_ptr<RHI::RHITexture> mpTexture;
        std::unique_ptr<RHI::RHIDescriptor> mpSRV;
        std::vector<std::unique_ptr<RHI::RHIDescriptor>> mUAVs;
    };
}