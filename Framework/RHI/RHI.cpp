#include "RHI.hpp"
#include "RHIVulkan/RHIDeviceVK.hpp"

namespace RHI
{
    RHIDevice *CreateRHIDevice(const RHIDeviceDesc &desc)
    {
        RHIDevice* device = nullptr;

        switch (desc.RenderBackend)
        {
        case ERHIRenderBackend::Vulkan:
            device = new RHI::Vulkan::RHIDeviceVK(desc);
            if (!((RHI::Vulkan::RHIDeviceVK*)device)->Initialize())
            {
                delete device;
                device = nullptr;
            }
            break;
        default:
            break;
        }
        return device;
    }

    uint32_t GetFormatRowPitch(ERHIFormat format, uint32_t width)
    {
        switch (format)
        {
        case ERHIFormat::RGBA32F:
        case ERHIFormat::RGBA32UI:
        case ERHIFormat::RGBA32SI:
            return width * 16;
        case ERHIFormat::RGB32F:
        case ERHIFormat::RGB32UI:
        case ERHIFormat::RGB32SI:
            return width * 12;
        case ERHIFormat::RGBA16F:
        case ERHIFormat::RGBA16UI:
        case ERHIFormat::RGBA16SI:
        case ERHIFormat::RGBA16UNORM:
        case ERHIFormat::RGBA16SNORM:
            return width * 8;
        case ERHIFormat::RGBA8UI:
        case ERHIFormat::RGBA8SI:
        case ERHIFormat::RGBA8UNORM:
        case ERHIFormat::RGBA8SNORM:
        case ERHIFormat::RGBA8SRGB:
        case ERHIFormat::BGRA8UNORM:
        case ERHIFormat::BGRA8SRGB:
        case ERHIFormat::RGB10A2UNORM:
        case ERHIFormat::R11G11B10F:
        case ERHIFormat::RGB9E5:
            return width * 4;
        case ERHIFormat::RG32F:
        case ERHIFormat::RG32UI:
        case ERHIFormat::RG32SI:
            return width * 8;
        case ERHIFormat::RG16F:
        case ERHIFormat::RG16UI:
        case ERHIFormat::RG16SI:
        case ERHIFormat::RG16UNORM:
        case ERHIFormat::RG16SNORM:
            return width * 4;
        case ERHIFormat::RG8UI:
        case ERHIFormat::RG8SI:
        case ERHIFormat::RG8UNORM:
        case ERHIFormat::RG8SNORM:
            return width * 2;
        case ERHIFormat::R32F:
        case ERHIFormat::R32UI:
        case ERHIFormat::R32SI:
            return width * 4;
        case ERHIFormat::R16F:
        case ERHIFormat::R16UI:
        case ERHIFormat::R16SI:
        case ERHIFormat::R16UNORM:
        case ERHIFormat::R16SNORM:
            return width * 2;
        case ERHIFormat::R8UI:
        case ERHIFormat::R8SI:
        case ERHIFormat::R8UNORM:
        case ERHIFormat::R8SNORM:
            return width;
        case ERHIFormat::BC1UNORM:
        case ERHIFormat::BC1SRGB:
        case ERHIFormat::BC4UNORM:
        case ERHIFormat::BC4SNORM:
            return width / 2;
        case ERHIFormat::BC2UNORM:
        case ERHIFormat::BC2SRGB:
        case ERHIFormat::BC3UNORM:
        case ERHIFormat::BC3SRGB:
        case ERHIFormat::BC5UNORM:
        case ERHIFormat::BC5SNORM:
        case ERHIFormat::BC6U16F:
        case ERHIFormat::BC6S16F:
        case ERHIFormat::BC7UNORM:
        case ERHIFormat::BC7SRGB:
            return width;
        default:
            assert(false);
            return 0;
        }
    }

    uint32_t GetFormatBlockWidth(ERHIFormat format)
    {
        if (format >= ERHIFormat::BC1UNORM && format <= ERHIFormat::BC7SRGB)
        {
            return 4;
        }

    return 1;
    }

    uint32_t GetFormatBlockHeight(ERHIFormat format)
    {
        if (format >= ERHIFormat::BC1UNORM && format <= ERHIFormat::BC7SRGB)
        {
            return 4;
        }

        return 1;
    }

    uint32_t GetFormatComponentNum(ERHIFormat format)
    {
        switch (format)
        {
        case ERHIFormat::RGBA32F:
        case ERHIFormat::RGBA32UI:
        case ERHIFormat::RGBA32SI:
        case ERHIFormat::RGBA16F:
        case ERHIFormat::RGBA16UI:
        case ERHIFormat::RGBA16SI:
        case ERHIFormat::RGBA16UNORM:
        case ERHIFormat::RGBA16SNORM:
        case ERHIFormat::RGBA8UI:
        case ERHIFormat::RGBA8SI:
        case ERHIFormat::RGBA8UNORM:
        case ERHIFormat::RGBA8SNORM:
        case ERHIFormat::RGBA8SRGB:
        case ERHIFormat::BGRA8UNORM:
        case ERHIFormat::BGRA8SRGB:
        case ERHIFormat::RGB10A2UI:
        case ERHIFormat::RGB10A2UNORM:
            return 4;
        case ERHIFormat::RGB32F:
        case ERHIFormat::RGB32UI:
        case ERHIFormat::RGB32SI:
        case ERHIFormat::R11G11B10F:
        case ERHIFormat::RGB9E5:
            return 3;
        case ERHIFormat::RG32F:
        case ERHIFormat::RG32UI:
        case ERHIFormat::RG32SI:
        case ERHIFormat::RG16F:
        case ERHIFormat::RG16UI:
        case ERHIFormat::RG16SI:
        case ERHIFormat::RG16UNORM:
        case ERHIFormat::RG16SNORM:
        case ERHIFormat::RG8UI:
        case ERHIFormat::RG8SI:
        case ERHIFormat::RG8UNORM:
        case ERHIFormat::RG8SNORM:
            return 2;
        case ERHIFormat::R32F:
        case ERHIFormat::R32UI:
        case ERHIFormat::R32SI:
        case ERHIFormat::R16F:
        case ERHIFormat::R16UI:
        case ERHIFormat::R16SI:
        case ERHIFormat::R16UNORM:
        case ERHIFormat::R16SNORM:
        case ERHIFormat::R8UI:
        case ERHIFormat::R8SI:
        case ERHIFormat::R8UNORM:
        case ERHIFormat::R8SNORM:
            return 1;
        default:
            assert(false);
            return 0;
        }
    }

    bool IsDepthFormat(ERHIFormat format)
    {
        return format == ERHIFormat::D32FS8 || format == ERHIFormat::D32F || format == ERHIFormat::D16;
    }

    bool IsStencilFormat(ERHIFormat format)
    {
        return format == ERHIFormat::D32FS8;
    }

    bool IsSRGBFormat(ERHIFormat format)
    {
        return format == ERHIFormat::RGBA8SRGB || format == ERHIFormat::BGRA8SRGB;
    }

    uint32_t CalcSubresource(const RHITextureDesc &desc, uint32_t mipLevel, uint32_t arraySlice)
    {
        return mipLevel + desc.MipLevels * arraySlice;
    }

    void DecomposeSubresource(const RHITextureDesc &desc, uint32_t subresource, uint32_t &mipLevel, uint32_t &arraySlice)
    {
        mipLevel = subresource % desc.MipLevels;
        arraySlice = (subresource / desc.MipLevels) % desc.ArraySize;
    }
}