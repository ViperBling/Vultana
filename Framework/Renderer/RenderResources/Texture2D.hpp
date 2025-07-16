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

        RHI::RHITexture* GetTexture() const { return m_pTexture.get(); }
        RHI::RHIDescriptor* GetSRV() const { return m_pSRV.get(); }
        RHI::RHIDescriptor* GetUAV(uint32_t mip = 0) const;
    
    protected:
        eastl::string m_Name;

        eastl::unique_ptr<RHI::RHITexture> m_pTexture;
        eastl::unique_ptr<RHI::RHIDescriptor> m_pSRV;
        eastl::vector<eastl::unique_ptr<RHI::RHIDescriptor>> m_UAVs;
    };
}