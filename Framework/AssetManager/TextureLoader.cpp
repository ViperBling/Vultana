#include "TextureLoader.hpp"
#include "Utilities/Log.hpp"

#include "ddspp/ddspp.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fstream>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

static inline RHI::ERHITextureType GetTextureType(ddspp::TextureType type, bool array)
{
    switch (type)
    {
    case ddspp::Texture2D:
        return array ? RHI::ERHITextureType::Texture2DArray : RHI::ERHITextureType::Texture2D;
    case ddspp::Texture3D:
        return RHI::ERHITextureType::Texture3D;
    case ddspp::Cubemap:
        return array ? RHI::ERHITextureType::TextureCubeArray : RHI::ERHITextureType::TextureCube;
    default:
        return RHI::ERHITextureType::Texture2D;
    }
}

static inline RHI::ERHIFormat GetTextureFormat(ddspp::DXGIFormat format, bool srgb)
{
    switch(format)
    {
    case ddspp::UNKNOWN:
        return RHI::ERHIFormat::Unknown;
    case ddspp::R32G32B32A32_FLOAT:
        return RHI::ERHIFormat::RGBA32F;
    case ddspp::R32G32B32A32_UINT:
        return RHI::ERHIFormat::RGBA32UI;
    case ddspp::R32G32B32A32_SINT:
        return RHI::ERHIFormat::RGBA32SI;
    case ddspp::R16G16B16A16_FLOAT:
        return RHI::ERHIFormat::RGBA16F;
    case ddspp::R16G16B16A16_UINT:
        return RHI::ERHIFormat::RGBA16UI;
    case ddspp::R16G16B16A16_SINT:
        return RHI::ERHIFormat::RGBA16SI;
    case ddspp::R16G16B16A16_UNORM:
        return RHI::ERHIFormat::RGBA16UNORM;
    case ddspp::R16G16B16A16_SNORM:
        return RHI::ERHIFormat::RGBA16SNORM;
    case ddspp::R8G8B8A8_UINT:
        return RHI::ERHIFormat::RGBA8UI;
    case ddspp::R8G8B8A8_SINT:
        return RHI::ERHIFormat::RGBA8SI;
    case ddspp::R8G8B8A8_UNORM:
        return srgb ? RHI::ERHIFormat::RGBA8SRGB : RHI::ERHIFormat::RGBA8UNORM;
    case ddspp::R8G8B8A8_SNORM:
        return RHI::ERHIFormat::RGBA8SNORM;
    case ddspp::R8G8B8A8_UNORM_SRGB:
        return RHI::ERHIFormat::RGBA8SRGB;
    case ddspp::B8G8R8A8_UNORM:
        return srgb ? RHI::ERHIFormat::BGRA8SRGB : RHI::ERHIFormat::BGRA8UNORM;
    case ddspp::B8G8R8A8_UNORM_SRGB:
        return RHI::ERHIFormat::BGRA8SRGB;
    case ddspp::R9G9B9E5_SHAREDEXP:
        return RHI::ERHIFormat::RGB9E5;
    case ddspp::R32G32_FLOAT:
        return RHI::ERHIFormat::RG32F;
    case ddspp::R32G32_UINT:
        return RHI::ERHIFormat::RG32UI;
    case ddspp::R32G32_SINT:
        return RHI::ERHIFormat::RG32SI;
    case ddspp::R16G16_FLOAT:
        return RHI::ERHIFormat::RG16F;
    case ddspp::R16G16_UINT:
        return RHI::ERHIFormat::RG16UI;
    case ddspp::R16G16_SINT:
        return RHI::ERHIFormat::RG16SI;
    case ddspp::R16G16_UNORM:
        return RHI::ERHIFormat::RG16UNORM;
    case ddspp::R16G16_SNORM:
        return RHI::ERHIFormat::RG16SNORM;
    case ddspp::R8G8_UINT:
        return RHI::ERHIFormat::RG8UI;
    case ddspp::R8G8_SINT:
        return RHI::ERHIFormat::RG8SI;
    case ddspp::R8G8_UNORM:
        return RHI::ERHIFormat::RG8UNORM;
    case ddspp::R8G8_SNORM:
        return RHI::ERHIFormat::RG8SNORM;
    case ddspp::R32_FLOAT:
        return RHI::ERHIFormat::R32F;
    case ddspp::R32_UINT:
        return RHI::ERHIFormat::R32UI;
    case ddspp::R32_SINT:
        return RHI::ERHIFormat::R32SI;
    case ddspp::R16_FLOAT:
        return RHI::ERHIFormat::R16F;
    case ddspp::R16_UINT:
        return RHI::ERHIFormat::R16UI;
    case ddspp::R16_SINT:
        return RHI::ERHIFormat::R16SI;
    case ddspp::R16_UNORM:
        return RHI::ERHIFormat::R16UNORM;
    case ddspp::R16_SNORM:
        return RHI::ERHIFormat::R16SNORM;
    case ddspp::R8_UINT:
        return RHI::ERHIFormat::R8UI;
    case ddspp::R8_SINT:
        return RHI::ERHIFormat::R8SI;
    case ddspp::R8_UNORM:
        return RHI::ERHIFormat::R8UNORM;
    case ddspp::R8_SNORM:
        return RHI::ERHIFormat::R8SNORM;
    case ddspp::BC1_UNORM:
        return srgb ? RHI::ERHIFormat::BC1SRGB : RHI::ERHIFormat::BC1UNORM;
    case ddspp::BC1_UNORM_SRGB:
        return RHI::ERHIFormat::BC1SRGB;
    case ddspp::BC2_UNORM:
        return srgb ? RHI::ERHIFormat::BC2SRGB : RHI::ERHIFormat::BC2UNORM;
    case ddspp::BC2_UNORM_SRGB:
        return RHI::ERHIFormat::BC2SRGB;
    case ddspp::BC3_UNORM:
        return srgb ? RHI::ERHIFormat::BC3SRGB : RHI::ERHIFormat::BC3UNORM;
    case ddspp::BC3_UNORM_SRGB:
        return RHI::ERHIFormat::BC3SRGB;
    case ddspp::BC4_UNORM:
        return RHI::ERHIFormat::BC4UNORM;
    case ddspp::BC4_SNORM:
        return RHI::ERHIFormat::BC4SNORM;
    case ddspp::BC5_UNORM:
        return RHI::ERHIFormat::BC5UNORM;
    case ddspp::BC5_SNORM:
        return RHI::ERHIFormat::BC5SNORM;
    case ddspp::BC6H_UF16:
        return RHI::ERHIFormat::BC6U16F;
    case ddspp::BC6H_SF16:
        return RHI::ERHIFormat::BC6S16F;
    case ddspp::BC7_UNORM:
        return srgb ? RHI::ERHIFormat::BC7SRGB : RHI::ERHIFormat::BC7UNORM;
    case ddspp::BC7_UNORM_SRGB:
        return RHI::ERHIFormat::BC7SRGB;
    default:
        return RHI::ERHIFormat::Unknown;
    }
}

namespace Assets
{
    TextureLoader::~TextureLoader()
    {
        if (mpDecompressedData)
        {
            stbi_image_free(mpDecompressedData);
        }
    }

    bool TextureLoader::Load(const std::string &filename, bool srgb)
    {
        std::ifstream is;
        is.open(filename.c_str(), std::ios::binary);
        if (is.fail())
        {
            VTNA_LOG_DEBUG("[TextureLoader::Load] failed to open file: {}", filename);
            return false;
        }

        is.seekg(0, std::ios::end);
        uint32_t length = static_cast<uint32_t>(is.tellg());
        is.seekg(0, std::ios::beg);

        mFileData.resize(length);
        char* buffer = reinterpret_cast<char*>(mFileData.data());

        is.read(buffer, length);
        is.close();

        if (filename.find(".dds") != std::string::npos)
        {
            return LoadDDS(srgb);
        }
        else
        {
            return LoadSTB(srgb);
        }
    }

    bool TextureLoader::Resize(uint32_t width, uint32_t height)
    {
        if (mpDecompressedData == nullptr)
        {
            return false;
        }

        unsigned char* outputData = (unsigned char*)malloc(width * height * 4);
        auto res = stbir_resize_uint8_linear((const unsigned char*)mpDecompressedData, mWidth, mHeight, 0, outputData, width, height, 0, STBIR_RGBA);
        if (!res || outputData == nullptr)
        {
            free(outputData);
            return false;
        }
        stbi_image_free(mpDecompressedData);

        mpDecompressedData = outputData;
        mWidth = width;
        mHeight = height;
        return true;
    }

    bool TextureLoader::LoadDDS(bool srgb)
    {
        uint8_t* data = mFileData.data();

        ddspp::Descriptor desc;
        ddspp::Result res = ddspp::decode_header(data, desc);
        if (res != ddspp::Success)
        {
            VTNA_LOG_DEBUG("[TextureLoader::LoadDDS] failed to decode header");
            return false;
        }
        mWidth = desc.width;
        mHeight = desc.height;
        mDepth = desc.depth;
        mMipLevels = desc.numMips;
        mArraySize = desc.arraySize;
        mFormat = GetTextureFormat(desc.format, srgb);
        mType = GetTextureType(desc.type, desc.arraySize > 1);

        mpTextureData = data + desc.headerSize;
        mTextureSize = (uint32_t)mFileData.size() - desc.headerSize;

        return true;
    }

    bool TextureLoader::LoadSTB(bool srgb)
    {
        int x, y, comp;
        stbi_info_from_memory((stbi_uc*)mFileData.data(), static_cast<int>(mFileData.size()), &x, &y, &comp);

        bool isHDR = stbi_is_hdr_from_memory((stbi_uc*)mFileData.data(), static_cast<int>(mFileData.size()));
        bool is16Bits = stbi_is_16_bit_from_memory((stbi_uc*)mFileData.data(), static_cast<int>(mFileData.size()));
        int desiredChannels = comp == 3 ? 4 : 0;

        if (isHDR)
        {
            mpDecompressedData = stbi_loadf_from_memory((stbi_uc*)mFileData.data(), static_cast<int>(mFileData.size()), &x, &y, &comp, desiredChannels);

            switch (comp)
            {
            case 1:
                mFormat = RHI::ERHIFormat::R32F;
                break;
            case 2:
                mFormat = RHI::ERHIFormat::RG32F;
                break;
            case 3:
            case 4:
                mFormat = RHI::ERHIFormat::RGBA32F;
                break;
            default:
                assert(false);
                break;
            }
        }
        else if (is16Bits)
        {
            mpDecompressedData = stbi_load_16_from_memory((stbi_uc*)mFileData.data(), static_cast<int>(mFileData.size()), &x, &y, &comp, desiredChannels);

            switch (comp)
            {
            case 1:
                mFormat = RHI::ERHIFormat::R16UNORM;
                break;
            case 2:
                mFormat = RHI::ERHIFormat::RG16UNORM;
                break;
            case 3:
            case 4:
                mFormat = RHI::ERHIFormat::RGBA16UNORM;
                break;
            default:
                assert(false);
                break;
            }
        }
        else
        {
            mpDecompressedData = stbi_load_from_memory((stbi_uc*)mFileData.data(), static_cast<int>(mFileData.size()), &x, &y, &comp, desiredChannels);

            switch (comp)
            {
            case 1:
                mFormat = RHI::ERHIFormat::R8UNORM;
                break;
            case 2:
                mFormat = RHI::ERHIFormat::RG8UNORM;
                break;
            case 3:
            case 4:
                mFormat = srgb ? RHI::ERHIFormat::RGBA8SRGB : RHI::ERHIFormat::RGBA8UNORM;
                break;
            default:
                assert(false);
                break;
            }
        }
        if (mpDecompressedData == nullptr)
        {
            VTNA_LOG_DEBUG("[TextureLoader::LoadSTB] failed to load image");
            return false;
        }
        mWidth = x;
        mHeight = y;
        mTextureSize = GetFormatRowPitch(mFormat, mWidth) * mHeight;

        return true;
    }
}