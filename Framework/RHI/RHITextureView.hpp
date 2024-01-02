#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    struct TextureViewCreateInfo
    {
        RHITextureViewType Type;
        RHITextureViewDimension Dimension;
        RHITextureType TextureType;
        uint8_t BaseMipLevel;
        uint8_t MipLevelCount;
        uint8_t BaseArrayLayer;
        uint8_t ArrayLayerCount;
    };

    class RHITextureView
    {
    public:
        NOCOPY(RHITextureView)
        virtual ~RHITextureView() = default;

        virtual void Destroy() = 0;

    protected:
        explicit RHITextureView(const TextureViewCreateInfo& createInfo) {}
    };
}