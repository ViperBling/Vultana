#pragma once

#include "RHI/RHI.hpp"

#include <EASTL/unique_ptr.h>

namespace Renderer
{
    class FRendererBase;
}

namespace RenderResources
{
    class FTexture2D
    {
    public:
        FTexture2D(const eastl::string& name);

        bool Create(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags);

        RHI::FRHITexture* GetTexture() const { return m_pTexture.get(); }
        RHI::FRHIDescriptor* GetSRV() const { return m_pSRV.get(); }
        RHI::FRHIDescriptor* GetUAV(uint32_t mip = 0) const;
    
    protected:
        eastl::string m_Name;

        eastl::unique_ptr<RHI::FRHITexture> m_pTexture;
        eastl::unique_ptr<RHI::FRHIDescriptor> m_pSRV;
        eastl::vector<eastl::unique_ptr<RHI::FRHIDescriptor>> m_UAVs;
    };
}