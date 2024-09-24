#include "Texture2D.hpp"

#include "Core/VultanaEngine.hpp"

namespace Renderer
{
    Texture2D::Texture2D(const std::string &name)
    {
        mName = name;
    }

    bool Texture2D::Create(uint32_t width, uint32_t height, uint32_t levels, RHI::ERHIFormat format, RHI::ERHITextureUsageFlags flags)
    {
        RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
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
        mpTexture.reset(pDevice->CreateTexture(desc, mName));
        if (mpTexture == nullptr) return false;

        RHI::RHIShaderResourceViewDesc srvDesc;
        srvDesc.Format = format;
        mpSRV.reset(pDevice->CreateShaderResourceView(mpTexture.get(), srvDesc, mName + "_SRV"));
        if (mpSRV == nullptr) return false;

        if (flags & RHI::RHITextureUsageUnorderedAccess)
        {
            for (uint32_t i = 0; i < levels; i++)
            {
                RHI::RHIUnorderedAccessViewDesc uavDesc;
                uavDesc.Format = format;
                uavDesc.Texture.MipSlice = i;
                auto uav = pDevice->CreateUnorderedAccessView(mpTexture.get(), uavDesc, mName + "_UAV");
                if (uav == nullptr) return false;
                mUAVs.emplace_back(uav);
            }
        }
        return true;
    }

    RHI::RHIDescriptor *Texture2D::GetUAV(uint32_t mip) const
    {
        return mUAVs[mip].get();
    }
}