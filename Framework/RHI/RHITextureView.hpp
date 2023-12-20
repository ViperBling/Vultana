#pragma once

#include "RHICommon.hpp"

namespace Vultana
{
    struct TextureViewCreateInfo
    {
        RHITextureViewType Type;
        RHITextureViewDimension Dimension;
        RHITextureType TextureType;
        uint8_t BaseMipLevel;
        uint8_t MipLevels;
        uint8_t BaseArrayLayer;
        uint8_t ArrayLayers;
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