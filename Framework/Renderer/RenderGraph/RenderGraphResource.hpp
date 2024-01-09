#pragma once

#include "DAG.hpp"
#include "RHI/RHIPCH.hpp"

namespace Renderer
{
    class RenderGraphResource
    {
    public:


    private:
        std::string mName;

        DAGNodeID mFirstPass = UINT32_MAX;
        DAGNodeID mLastPass = 0;

        RHI::RHIBufferState mLastBufferState = RHI::RHIBufferState::Undefined;
        RHI::RHITextureState mLastTextureState = RHI::RHITextureState::Undefined;

        bool mbImported = false;
        bool mbExported = false;
    };

    class RGTexture : public RenderGraphResource
    {
    public:
    
    private:
        RHI::TextureCreateInfo mDesc;
        RHI::RHITexture* mTexture = nullptr;
        RHI::RHITextureUsageFlags mUsageFlags = RHI::RHITextureUsageBits::Count;
    };
    
}