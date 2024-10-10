#pragma once

#include "RHI/RHI.hpp"

namespace Assets
{
    class TextureLoader
    {
    public:
        TextureLoader() = default;
        ~TextureLoader();

        bool Load(const std::string& filename, bool srgb);

        uint32_t GetWidth() const { return mWidth; }
        uint32_t GetHeight() const { return mHeight; }
        uint32_t GetDepth() const { return mDepth; }
        uint32_t GetMipLevels() const { return mMipLevels; }
        uint32_t GetArraySize() const { return mArraySize; }
        RHI::ERHIFormat GetFormat() const { return mFormat; }
        RHI::ERHITextureType GetType() const { return mType; }

        void* GetData() const { return mpDecompressedData != nullptr ? mpDecompressedData : mpTextureData; }
        uint32_t GetDataSize() const { return mTextureSize; }

        bool Resize(uint32_t width, uint32_t height);

    private:
        bool LoadDDS(bool srgb);
        bool LoadSTB(bool srgb);

    private:
        uint32_t mWidth = 1;
        uint32_t mHeight = 1;
        uint32_t mDepth = 1;
        uint32_t mMipLevels = 1;
        uint32_t mArraySize = 1;
        RHI::ERHIFormat mFormat = RHI::ERHIFormat::Unknown;
        RHI::ERHITextureType mType = RHI::ERHITextureType::Texture2D;

        void* mpTextureData = nullptr;
        void* mpDecompressedData = nullptr;
        uint32_t mTextureSize = 0;

        std::vector<uint8_t> mFileData;
    };
}