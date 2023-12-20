#pragma once

#include "Utilities/Math.hpp"
#include "RHICommon.hpp"

namespace Vultana
{
    struct TextureViewCreateInfo;
    class RHITextureView;
    class RHIDevice;

    struct TextureCreateInfo
    {
        RHITextureDimension Dimension;
        Vector2 Extent;
        RHIFormat Format;
        RHITextureUsageFlags Usage;
        uint8_t MipLevels;
        uint8_t Samples;
        RHITextureState InitialState;
        std::string Name;
    };

    class RHITexture
    {
    public:
        NOCOPY(RHITexture)
        virtual ~RHITexture() = default;

        virtual RHITextureView* CreateTextureView(const TextureViewCreateInfo& createInfo) = 0;
        virtual void Destroy() = 0;
    
    protected:
        RHITexture() = default;
        explicit RHITexture(const TextureCreateInfo& createInfo) {}
    };
}