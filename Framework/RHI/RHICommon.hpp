#pragma once

#include <string>
#include <vector>

namespace Vultana
{
    class RHIBuffer;
    class RHITexture;
    class RHIShader;
    class RHIHeap;

    static const uint32_t RHI_MAX_IN_FLIGHT_FRAMES = 3;
    static const uint32_t RHI_MAX_ROOT_CONSTANT = 8;
    static const uint32_t RHI_MAX_CBV_BINDINGS = 3;

    enum class RHIDeviceType
    {
        CPU = 0,
        DISCRETE_GPU,
        INTEGRATED_GPU,
        VIRTUAL_GPU,
        OTHER,
    };

    enum class RHIRenderBackend
    {
        Vulkan,
        Count
    };

    enum class RHIFormat
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
    };

    enum class RHIMemoryType
    {
        GPUOnly,
        CPUOnly,
        CPUToGPU,
        GPUToCPU
    };

    enum class RHIAllocationType
    {
        Committed,
        Placed,
        Sparse
    };

    enum RHIBufferUsageBit
    {
        RHIBufferUsageConstantBuffer = 1 << 0,
        RHIBufferUsageStructuredBuffer = 1 << 1,
        RHIBufferUsageTypedBuffer = 1 << 2,
        RHIBufferUsageRawBuffer = 1 << 3,
        RHIBufferUsageUnorderedAccess = 1 << 4,
    };
    using RHIBufferUsageFlags = uint32_t;

    enum RHITextureUsageBit
    {
        RHITextureUsageRenderTarget = 1 << 0,
        RHITextureUsageDepthStencil = 1 << 1,
        RHITextureUsageUnorderedAccess = 1 << 2,
    };
    using RHITextureUsageFlags = uint32_t;

    enum class RHITextureType
    {
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube,
        TextureCubeArray,
        Count
    };

    enum class RHICommandQueue
    {
        Graphics,
        Compute,
        Copy,
        Count
    };

    struct RHIBufferDesc
    {
        uint32_t            Stride = 1;
        uint32_t            Size = 1;
        RHIFormat           Format = RHIFormat::Unknown;
        RHIMemoryType       MemoryType = RHIMemoryType::GPUOnly;
        RHIAllocationType   AllocType = RHIAllocationType::Placed;
        RHIBufferUsageFlags Usage = 0;
        RHIHeap*            Heap = nullptr;
        uint32_t            HeapOffset = 0;
    };

    inline bool operator==(const RHIBufferDesc &lhs, const RHIBufferDesc &rhs)
    {
        return lhs.Stride == rhs.Stride &&
               lhs.Size == rhs.Size &&
               lhs.Format == rhs.Format &&
               lhs.MemoryType == rhs.MemoryType &&
               lhs.AllocType == rhs.AllocType &&
               lhs.Usage == rhs.Usage;
    }

    struct RHIDeviceInfo
    {
        RHIRenderBackend Backend = RHIRenderBackend::Vulkan;
        uint32_t MaxFrameLag = 3;
    };

    enum class RHIPipelineType
    {
        Graphics,
        Compute,
        Count
    };
}
