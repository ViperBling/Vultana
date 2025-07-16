#pragma once

#include "RHI/RHI.hpp"

namespace Assets
{
    class TextureLoader
    {
    public:
        TextureLoader() = default;
        ~TextureLoader();

        bool Load(const eastl::string& filename, bool srgb);

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        uint32_t GetDepth() const { return m_Depth; }
        uint32_t GetMipLevels() const { return m_MipLevels; }
        uint32_t GetArraySize() const { return m_ArraySize; }
        RHI::ERHIFormat GetFormat() const { return m_Format; }
        RHI::ERHITextureType GetType() const { return m_Type; }

        void* GetData() const { return m_pDecompressedData != nullptr ? m_pDecompressedData : m_pTextureData; }
        uint32_t GetDataSize() const { return m_TextureSize; }

        bool Resize(uint32_t width, uint32_t height);

    private:
        bool LoadDDS(bool srgb);
        bool LoadSTB(bool srgb);

    private:
        uint32_t m_Width = 1;
        uint32_t m_Height = 1;
        uint32_t m_Depth = 1;
        uint32_t m_MipLevels = 1;
        uint32_t m_ArraySize = 1;
        RHI::ERHIFormat m_Format = RHI::ERHIFormat::Unknown;
        RHI::ERHITextureType m_Type = RHI::ERHITextureType::Texture2D;

        void* m_pTextureData = nullptr;
        void* m_pDecompressedData = nullptr;
        uint32_t m_TextureSize = 0;

        eastl::vector<uint8_t> m_FileData;
    };
}