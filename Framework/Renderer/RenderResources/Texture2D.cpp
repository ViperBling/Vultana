#include "Texture2D.hpp"

#include "Core/VultanaEngine.hpp"

namespace RenderResources
{
    Texture2D::Texture2D(const eastl::string &name)
    {
        m_Name = name;
    }

    bool Texture2D::Create(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags)
    {
        Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        RHI::RHIDevice* pDevice = pRenderer->GetDevice();

        RHI::RHITextureDesc desc {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = levels;
        desc.Format = format;
        desc.Usage = flags;

        if ((flags & RHI::RHITextureUsageRenderTarget) || (flags & RHI::RHITextureUsageDepthStencil) || (flags & RHI::RHITextureUsageUnorderedAccess))
        {
            desc.AllocationType = RHI::ERHIAllocationType::Committed;
        }
        m_pTexture.reset(pDevice->CreateTexture(desc, m_Name));
        if (m_pTexture == nullptr) return false;

        RHI::RHIShaderResourceViewDesc srvDesc;
        srvDesc.Format = format;
        m_pSRV.reset(pDevice->CreateShaderResourceView(m_pTexture.get(), srvDesc, m_Name + "_SRV"));
        if (m_pSRV == nullptr) return false;

        if (flags & RHI::RHITextureUsageUnorderedAccess)
        {
            for (uint32_t i = 0; i < levels; i++)
            {
                RHI::RHIUnorderedAccessViewDesc uavDesc;
                uavDesc.Format = format;
                uavDesc.Texture.MipSlice = i;
                auto uav = pDevice->CreateUnorderedAccessView(m_pTexture.get(), uavDesc, m_Name + "_UAV");
                if (uav == nullptr) return false;
                m_UAVs.emplace_back(uav);
            }
        }
        return true;
    }

    RHI::RHIDescriptor *Texture2D::GetUAV(uint32_t mip) const
    {
        return m_UAVs[mip].get();
    }
}