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

        TextureViewCreateInfo()
            : Type(RHITextureViewType::Count)
            , Dimension(RHITextureViewDimension::Count)
            , TextureType(RHITextureType::Count)
            , BaseMipLevel(0)
            , MipLevelCount(0)
            , BaseArrayLayer(0)
            , ArrayLayerCount(0)
        {}
        TextureViewCreateInfo& SetType(RHITextureViewType inType)
        {
            Type = inType;
            return *this;
        }
        TextureViewCreateInfo& SetDimension(RHITextureViewDimension inDimension)
        {
            Dimension = inDimension;
            return *this;
        }
        TextureViewCreateInfo& SetTextureType(RHITextureType inTextureType)
        {
            TextureType = inTextureType;
            return *this;
        }
        TextureViewCreateInfo& SetMipLevels(uint8_t inBaseMipLevel, uint8_t inMipLevelCount)
        {
            BaseMipLevel = inBaseMipLevel;
            MipLevelCount = inMipLevelCount;
            return *this;
        }
        TextureViewCreateInfo& SetArrayLayer(uint8_t inBaseArrayLayer, uint8_t inArrayLayerCount)
        {
            BaseArrayLayer = inBaseArrayLayer;
            ArrayLayerCount = inArrayLayerCount;
            return *this;
        }

        size_t Hash() const
        {
            return Utility::HashUtils::CityHash(this, sizeof(TextureViewCreateInfo));
        }
    };

    class RHITextureView
    {
    public:
        NOCOPY(RHITextureView)
        virtual ~RHITextureView() = default;

    protected:
        explicit RHITextureView(const TextureViewCreateInfo& createInfo) {}
    };
}