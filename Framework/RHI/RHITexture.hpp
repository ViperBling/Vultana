#pragma once

#include "Utilities/Math.hpp"
#include "RHICommon.hpp"

namespace RHI
{
    struct TextureViewCreateInfo;
    class RHITextureView;
    class RHIDevice;

    struct TextureCreateInfo
    {
        RHITextureDimension Dimension;
        Math::Vector3u Extent;
        RHIFormat Format;
        RHITextureUsageFlags Usage;
        uint8_t MipLevels;
        uint8_t Samples;
        RHITextureState InitialState;
        std::string Name;

        TextureCreateInfo()
            : Dimension(RHITextureDimension::Texture2D)
            , Extent({ 1, 1, 1 })
            , Format(RHIFormat::Count)
            , Usage(RHITextureUsageFlags::null)
            , MipLevels(1)
            , Samples(1)
            , InitialState(RHITextureState::Count)
            , Name("Texture")
        {}

        TextureCreateInfo& SetDimension(RHITextureDimension inDimension)
        {
            Dimension = inDimension;
            return *this;
        }
        TextureCreateInfo& SetExtent(Math::Vector3u inExtent)
        {
            Extent = inExtent;
            return *this;
        }
        TextureCreateInfo& SetFormat(RHIFormat inFormat)
        {
            Format = inFormat;
            return *this;
        }
        TextureCreateInfo& SetUsage(RHITextureUsageFlags inUsage)
        {
            Usage = inUsage;
            return *this;
        }
        TextureCreateInfo& SetMipLevels(uint8_t inMipLevels)
        {
            MipLevels = inMipLevels;
            return *this;
        }
        TextureCreateInfo& SetSamples(uint8_t inSamples)
        {
            Samples = inSamples;
            return *this;
        }
        TextureCreateInfo& SetInitialState(RHITextureState inInitialState)
        {
            InitialState = inInitialState;
            return *this;
        }
        TextureCreateInfo& SetName(const std::string& inName)
        {
            Name = inName;
            return *this;
        }

        bool operator==(const TextureCreateInfo& other) const
        {
            return Dimension == other.Dimension &&
                Extent == other.Extent &&
                Format == other.Format &&
                Usage == other.Usage &&
                MipLevels == other.MipLevels &&
                Samples == other.Samples &&
                InitialState == other.InitialState;
        }
    };

    class RHITexture
    {
    public:
        NOCOPY(RHITexture)
        virtual ~RHITexture() = default;

        const TextureCreateInfo& GetCreateInfo() const { return mCreateInfo; }
        virtual std::unique_ptr<RHITextureView> CreateTextureView(const TextureViewCreateInfo& createInfo) = 0;
    
    protected:
        RHITexture() = default;
        explicit RHITexture(const TextureCreateInfo& createInfo) {}

        TextureCreateInfo mCreateInfo;
    };
}