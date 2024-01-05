#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <tcb/span.hpp>

namespace RHI
{
    static const uint32_t RHI_MAX_INFLIGHT_FRAMES = 3;

    enum class ERenderBackend
    {
        Vulkan,
        D3D12,
        Count,
    };

    enum class EPixelFormat
    {
        Unknown,

        RGBA32F,
        RGBA32UI,
        RGBA32SI,
        RGBA16F,
        RGBA16UI,
        RGBA16SI,
        RGBA16UNORM,
        RGBA16SNORM,
        RGBA8UI,
        RGBA8SI,
        RGBA8UNORM,
        RGBA8SNORM,
        RGBA8SRGB,
        BGRA8UNORM,
        BGRA8SRGB,
        RGB10A2UI,
        RGB10A2UNORM,

        RGB32F,
        RGB32UI,
        RGB32SI,
        R11G11B10F,
        RGB9E5,

        RG32F,
        RG32UI,
        RG32SI,
        RG16F,
        RG16UI,
        RG16SI,
        RG16UNORM,
        RG16SNORM,
        RG8UI,
        RG8SI,
        RG8UNORM,
        RG8SNORM,

        R32F,
        R32UI,
        R32SI,
        R16F,
        R16UI,
        R16SI,
        R16UNORM,
        R16SNORM,
        R8UI,
        R8SI,
        R8UNORM,
        R8SNORM,

        D32F,
        D32FS8,
        D16,

        BC1UNORM,
        BC1SRGB,
        BC2UNORM,
        BC2SRGB,
        BC3UNORM,
        BC3SRGB,
        BC4UNORM,
        BC4SNORM,
        BC5UNORM,
        BC5SNORM,
        BC6U16F,
        BC6S16F,
        BC7UNORM,
        BC7SRGB,

        Count,
    };

    enum class EMemoryType
    {
        GPUOnly,
        CPUOnly,
        CPUToGPU,
        GPUToCPU,
        Count,
    };

    enum class EAlloactionType
    {
        Committed,
        Placed,
        Sparse,
        Count,
    };

    enum class EBufferUsageBit
    {
        RHIBufferUsageConstantBuffer    = 1 << 0,
        RHIBufferUsageStructuredBuffer  = 1 << 1,
        RHIBufferUsageTypedBuffer       = 1 << 2,
        RHIBufferUsageRawBuffer         = 1 << 3,
        RHIBufferUsageUnorderedAccess   = 1 << 4,
    };
    using RHIBufferUsageFlags = uint32_t;

    enum class ETextureUsageBit
    {
        RHITextureUsageRenderTarget     = 1 << 1,
        RHITextureUsageDepthStencil     = 1 << 2,
        RHITextureUsageUnorderedAccess  = 1 << 3,
    };
    using RHITextureUsageFlags = uint32_t;

    enum class ETextureType
    {
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube,
        TextureCubeArray,
        Count,
    };

    enum class ECommandQueueType
    {
        Graphics,
        Compute,
        Copy,
        Count,
    };
}