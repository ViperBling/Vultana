#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <tcb/span.hpp>

namespace RHI
{
    class RHIHeap;

    static const uint32_t RHI_MAX_INFLIGHT_FRAMES = 3;

    enum class ERHIRenderBackend
    {
        Vulkan,
        D3D12,
        Count,
    };

    enum class ERHIFormat
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

    enum class ERHIMemoryType
    {
        GPUOnly,
        CPUOnly,
        CPUToGPU,
        GPUToCPU,
        Count,
    };

    enum class ERHIAlloactionType
    {
        Committed,
        Placed,
        Sparse,
        Count,
    };

    enum EBufferUsageBit
    {
        RHIBufferUsageConstantBuffer    = 1 << 0,
        RHIBufferUsageStructuredBuffer  = 1 << 1,
        RHIBufferUsageTypedBuffer       = 1 << 2,
        RHIBufferUsageRawBuffer         = 1 << 3,
        RHIBufferUsageUnorderedAccess   = 1 << 4,
    };
    using ERHIBufferUsageFlags = uint32_t;

    enum ETextureUsageBit
    {
        RHITextureUsageRenderTarget     = 1 << 1,
        RHITextureUsageDepthStencil     = 1 << 2,
        RHITextureUsageUnorderedAccess  = 1 << 3,
    };
    using ERHITextureUsageFlags = uint32_t;

    enum class ERHITextureType
    {
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube,
        TextureCubeArray,
        Count,
    };

    enum class ERHICommandQueueType
    {
        Graphics,
        Compute,
        Copy,
        Count,
    };

    enum ERHIAccessBit
    {
        RHIAccessPresent              = 1 << 0,
        RHIAccessRTV                  = 1 << 1,
        RHIAccessDSV                  = 1 << 2,
        RHIAccessDSVReadOnly          = 1 << 3,
        RHIAccessVertexShaderSRV      = 1 << 4,
        RHIAccessPixelShaderSRV       = 1 << 5,
        RHIAccessComputeSRV           = 1 << 6,
        RHIAccessVertexShaderUAV      = 1 << 7,
        RHIAccessPixelShaderUAV       = 1 << 8,
        RHIAccessComputeUAV           = 1 << 9,
        RHIAccessClearUAV             = 1 << 10,
        RHIAccessCopyDst              = 1 << 11,
        RHIAccessCopySrc              = 1 << 12,
        RHIAccessShadingRate          = 1 << 13,
        RHIAccessIndexBuffer          = 1 << 14,
        RHIAccessIndirectArgs         = 1 << 15,
        RHIAccessASRead               = 1 << 16,
        RHIAccessASWrite              = 1 << 17,
        RHIAccessDiscard              = 1 << 18, //aliasing barrier

        RHIAccessMaskVS     = RHIAccessVertexShaderSRV | RHIAccessVertexShaderUAV,
        RHIAccessMaskPS     = RHIAccessPixelShaderSRV | RHIAccessPixelShaderUAV,
        RHIAccessMaskCS     = RHIAccessComputeSRV | RHIAccessComputeUAV,
        RHIAccessMaskSRV    = RHIAccessVertexShaderSRV | RHIAccessPixelShaderSRV | RHIAccessComputeSRV,
        RHIAccessMaskUAV    = RHIAccessVertexShaderUAV | RHIAccessPixelShaderUAV | RHIAccessComputeUAV,
        RHIAccessMaskDSV    = RHIAccessDSV | RHIAccessDSVReadOnly,
        RHIAccessMaskCopy   = RHIAccessCopyDst | RHIAccessCopySrc,
        RHIAccessMaskAS     = RHIAccessASRead | RHIAccessASWrite,
    };
    using ERHIAccessFlags = uint32_t;

    enum class ERHIRenderPassLoadOp
    {
        Load,
        Clear,
        DontCare,
        Count,
    };

    enum class ERHIRenderPassStoreOp
    {
        Store,
        DontCare,
        Count,
    };

    enum class ERHIShaderResourceViewType
    {
        Textue2D,
        Texture2DArray,
        Texture3D,
        TextureCube,
        TextureCubeArray,
        StructuredBuffer,
        TypedBuffer,
        RawBuffer,
        Count,
    };

    enum class ERHIUnorderedAccessViewType
    {
        Texture2D,
        Texture2DArray,
        Texture3D,
        StructuredBuffer,
        TypedBuffer,
        RawBuffer,
        Count,
    };
    
    static const uint32_t RHI_ALL_SUB_RESOURCE = 0xFFFFFFFF;
    static const uint32_t RHI_INVALID_RESOURCE = 0xFFFFFFFF;

    struct RHIDeviceDesc
    {
        ERHIRenderBackend RenderBackend = ERHIRenderBackend::Vulkan;
        uint32_t MaxFrameLag = 3;
    };

    struct RHISwapchainDesc
    {
        void* WindowHandle = nullptr;
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint32_t BufferCount = 3;
        ERHIFormat ColorFormat = ERHIFormat::BGRA8SRGB;
    };

    struct RHIHeapDesc
    {
        uint32_t Size = 1;
        ERHIMemoryType MemoryType = ERHIMemoryType::GPUOnly;
    };

    struct RHIBufferDesc
    {
        uint32_t Stride = 1;
        uint32_t Size = 1;
        ERHIFormat Format = ERHIFormat::Unknown;
        ERHIMemoryType MemoryType = ERHIMemoryType::GPUOnly;
        ERHIAlloactionType AllocationType = ERHIAlloactionType::Placed;
        ERHIBufferUsageFlags Usage = 0;
        RHIHeap* Heap = nullptr;
        uint32_t HeapOffset = 0;
    };

    inline bool operator==(const RHIBufferDesc& lhs, const RHIBufferDesc& rhs)
    {
        return lhs.Stride == rhs.Stride &&
            lhs.Size == rhs.Size &&
            lhs.Format == rhs.Format &&
            lhs.MemoryType == rhs.MemoryType &&
            lhs.AllocationType == rhs.AllocationType &&
            lhs.Usage == rhs.Usage;
    }
}