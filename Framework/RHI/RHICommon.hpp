#pragma once

#include "Utilities/Utility.hpp"

#include <iostream>
#include <type_traits>
#include <memory>
#include <string>
#include <vector>

#define RHI_FLAGS_DECLARE(FlagsType, BitsType)      \
    FlagsType operator&(BitsType a, BitsType b);    \
    FlagsType operator&(FlagsType a, BitsType b);   \
    FlagsType operator|(BitsType a, BitsType b);    \
    FlagsType operator|(FlagsType a, BitsType b);   \

namespace Vultana
{
    static const uint32_t RHI_MAX_IN_FLIGHT_FRAMES = 3;
    static const uint32_t RHI_MAX_ROOT_CONSTANT = 8;
    static const uint32_t RHI_MAX_CBV_BINDINGS = 3;

    using EnumType = uint32_t;

    enum class RHIDeviceType : EnumType
    {
        Software,
        Hardware,
        Count
    };

    enum class RHIRenderBackend : EnumType
    {
        Vulkan,
        Count
    };

    enum class RHICommandQueueType : EnumType
    {
        Graphics,
        Compute,
        Transfer,
        Count
    };

    enum class RHIMapMode  : EnumType
    {
        Read,
        Write,
        Count
    };

    enum class RHIFormat : EnumType
    {
        // 8-Bits
        R8_UNORM,
        R8_SNORM,
        R8_UINT,
        R8_SINT,
        // 16-Bits
        R16_UINT,
        R16_SINT,
        R16_FLOAT,
        RG8_UNORM,
        RG8_SNORM,
        RG8_UINT,
        RG8_SINT,
        // 32-Bits
        R32_UINT,
        R32_SINT,
        R32_FLOAT,
        RG16_UINT,
        RG16_SINT,
        RG16_FLOAT,
        RGBA8_UNORM,
        RGBA8_UNORM_SRGB,
        RGBA8_SNORM,
        RGBA8_UINT,
        RGBA8_SINT,
        BGRA8_UNORM,
        BGRA8_UNORM_SRGB,
        RGB9E5_FLOAT,
        RGB10A2_UNORM,
        RG11B10_FLOAT,
        // 64-Bits
        RG32_UINT,
        RG32_SINT,
        RG32_FLOAT,
        RGBA16_UINT,
        RGBA16_SINT,
        RGBA16_FLOAT,
        // 128-Bits
        RGBA32_UINT,
        RGBA32_SINT,
        RGBA32_FLOAT,
        // Depth-Stencil
        D16_UNORM,
        D24_UNORM_S8_UINT,
        D32_FLOAT,
        D32_FLOAT_S8_UINT,
        // TODO features / bc / etc / astc
        Count
    };

    enum class RHIVertexFormat : EnumType
    {
        // 8-Bits Channel
        UINT_8X2,
        UINT_8X4,
        SINT_8X2,
        SINT_8X4,
        UNORM_8X2,
        UNORM_8X4,
        SNORM_8X2,
        SNORM_8X4,
        // 16-Bits Channel
        UINT_16X2,
        UINT_16X4,
        SINT_16X2,
        SINT_16X4,
        UNORM_16X2,
        UNORM_16X4,
        SNORM_16X2,
        SNORM_16X4,
        FLOAT_16X2,
        FLOAT_16X4,
        // 32-Bits Channel
        FLOAT_32X1,
        FLOAT_32X2,
        FLOAT_32X3,
        FLOAT_32X4,
        UINT_32X1,
        UINT_32X2,
        UINT_32X3,
        UINT_32X4,
        SINT_32X1,
        SINT_32X2,
        SINT_32X3,
        SINT_32X4,
        Count
    };

    enum class RHITextureDimension : EnumType
    {
        Texture1D,
        Texture2D,
        Texture3D,
        Count
    };

    enum class RHITextureViewDimension : EnumType
    {
        TextureView1D,
        TextureView2D,
        TextureView2DArray,
        TextureViewCube,
        TextureViewCubeArray,
        TextureView3D,
        Count
    };

    enum class RHITextureType : EnumType
    {
        Color,
        Depth,
        Stencil,
        DepthStencil,
        Count
    };

    enum class RHITextureViewType : EnumType
    {
        TextureBinding,
        StorageBinding,
        ColorAttachment,
        DepthStencil,
        Count
    };

    enum class RHIBufferViewType : EnumType
    {
        Vertex,
        Index,
        UniformBinding,
        StorageBinding,
        Count
    };

    enum class RHISamplerAddressMode : EnumType
    {
        Clamp,
        Repeat,
        Mirror,
        Count
    };

    enum class RHISamplerFilterMode : EnumType
    {
        Nearest,
        Linear,
        Count
    };

    enum class RHICompareOp : EnumType
    {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
        Count
    };

    enum class RHIHLSLBindingRangeType : EnumType
    {
        ConstantBuffer,
        Texture,
        Sampler,
        UnorderedAccess,
        Count
    };

    enum class RHIBindingType : EnumType
    {
        UniformBuffer,
        StorageBuffer,
        Sampler,
        Texture,
        StorageTexture,
        Count
    };

    enum class RHISamplerBindingType : EnumType
    {
        Filtering,
        NoFiltering,
        Comparsion,
        Count
    };

    enum class RHITextureSampleType : EnumType
    {
        FilterableFloat,
        NonFilterableFloat,
        Depth,
        Sint,
        Uint,
        Count
    };

    enum class RHIVertexStepMode : EnumType
    {
        PerVertex,
        PerInstance,
        Count
    };

    enum class RHIPrimitiveTopologyType : EnumType
    {
        Point,
        Line,
        Triangle,
        Count
    };

    enum class RHIPrimitiveTopology : EnumType
    {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        LineListAdj,
        LineStripAdj,
        TriangleListAdj,
        TriangleStripAdj,
        Count
    };

    enum class RHIIndexFormat : EnumType
    {
        UINT_16,
        UINT_32,
        Count
    };

    enum class RHIFrontFace : EnumType
    {
        CounterClockwise,
        Clockwise,
        Count
    };

    enum class RHICullMode : EnumType
    {
        None,
        Front,
        Back,
        Count
    };

    enum class RHIStencilOp : EnumType
    {
        Keep,
        Zero,
        Replace,
        Invert,
        IncrementClamp,
        DecrementClamp,
        IncrementWrap,
        DecrementWrap,
        Count
    };

    enum class RHIBlendFactor : EnumType
    {
        Zero,
        One,
        Src,
        OneMinusSrc,
        SrcAlpha,
        OneMinusSrcAlpha,
        Dst,
        OneMinusDst,
        DstAlpha,
        OneMinusDstAlpha,
        Count
    };

    enum class RHIBlendOp : EnumType
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
        Count
    };

    enum class RHILoadOp : EnumType
    {
        Load,
        Clear,
        Count
    };

    enum class RHIStoreOp : EnumType
    {
        Store,
        Discard,
        Count
    };

    enum class RHIPresentMode : EnumType
    {
        Immediate,
        Vsync,
        Count
    };

    enum class RHIResourceType : EnumType
    {
        Buffer,
        Texture,
        Count
    };

    enum class RHIBufferState : EnumType
    {
        Undefined,
        CopySrc,
        CopyDst,
        ShaderReadOnly,
        Storage,
        Count
    };

    enum class RHITextureState : EnumType
    {
        Undefined,
        CopySrc,
        CopyDst,
        ShaderReadOnly,
        RenderTarget,
        Storage,
        DepthStencilReadOnly,
        DepthStencilWrite,
        Present,
        Count
    };



    template <typename T = uint32_t>
    class RHIFlags
    {
    public:
        using UnderlyingType = T;

        RHIFlags() = default;
        ~RHIFlags() = default;
        RHIFlags(T value) : mValue(value) {}

        template <typename E>
        requires std::is_same_v<T, std::underlying_type_t<E>>
        RHIFlags(E e) : mValue(static_cast<T>(e)) {}

        T Value() const
        {
            return mValue;
        }

        explicit operator bool()
        {
            return mValue;
        }

        bool operator==(RHIFlags other) const
        {
            return mValue == other.mValue;
        }

        bool operator!=(RHIFlags other) const
        {
            return mValue != other.mValue;
        }

        bool operator==(T inValue) const
        {
            return mValue == inValue;
        }

        bool operator!=(T inValue) const
        {
            return mValue != inValue;
        }

        template <typename E>
        requires std::is_same_v<T, std::underlying_type_t<E>>
        bool operator==(E e) const
        {
            return mValue == static_cast<T>(e);
        }

        template <typename E>
        requires std::is_same_v<T, std::underlying_type_t<E>>
        bool operator!=(E e) const
        {
            return mValue != static_cast<T>(e);
        }

    private:
        T mValue;
    };

    template <typename T>
    RHIFlags<T> operator&(RHIFlags<T> a, RHIFlags<T> b)
    {
        return Flags<T>(a.Value() & b.Value());
    }

    template <typename T>
    RHIFlags<T> operator|(RHIFlags<T> a, RHIFlags<T> b)
    {
        return Flags<T>(a.Value() | b.Value());
    }

    using RHIBufferUsageFlags = RHIFlags<>;
    enum class RHIBufferUsageBits : RHIBufferUsageFlags::UnderlyingType
    {
        MapRead         = 0x1,
        MapWrite        = 0x2,
        CopySrc         = 0x4,
        CopyDst         = 0x8,
        Index           = 0x10,
        Vertex          = 0x20,
        Uniform         = 0x40,
        Storage         = 0x80,
        Indirect        = 0x100,
        QueryResolve    = 0x200,
        Count,
    };
    RHI_FLAGS_DECLARE(RHIBufferUsageFlags, RHIBufferUsageBits)

    using RHITextureUsageFlags = RHIFlags<>;
    enum class RHITextureUsageBits : RHITextureUsageFlags::UnderlyingType
    {
        CopySrc                 = 0x1,
        CopyDst                 = 0x2,
        TextureBinding          = 0x4,
        StorageBinding          = 0x8,
        RenderAttachment        = 0x10,
        DepthStencilAttachment  = 0x20,
        Count
    };
    RHI_FLAGS_DECLARE(RHITextureUsageFlags, RHITextureUsageBits)

    using RHIShaderStageFlags = RHIFlags<>;
    enum class RHIShaderStageBits : RHIShaderStageFlags::UnderlyingType
    {
        Vertex    = 0x1,
        Pixel     = 0x2,
        Compute   = 0x4,
        Geometry  = 0x8,
        Domain    = 0x10,
        Hull      = 0x20,
        Count
    };
    RHI_FLAGS_DECLARE(RHIShaderStageFlags, RHIShaderStageBits)

    using RHIColorWriteFlags = RHIFlags<>;
    enum class RHIColorWriteBits : RHIColorWriteFlags::UnderlyingType
    {
        Red   = 0x1,
        Green = 0x2,
        Blue  = 0x4,
        Alpha = 0x8,
        Count
    };
    RHI_FLAGS_DECLARE(RHIColorWriteFlags, RHIColorWriteBits)

    // enum class RHIMemoryType
    // {
    //     GPUOnly,
    //     CPUOnly,
    //     CPUToGPU,
    //     GPUToCPU
    // };

    // enum class RHIAllocationType
    // {
    //     Committed,
    //     Placed,
    //     Sparse
    // };

    // enum RHIBufferUsageBit
    // {
    //     RHIBufferUsageConstantBuffer = 1 << 0,
    //     RHIBufferUsageStructuredBuffer = 1 << 1,
    //     RHIBufferUsageTypedBuffer = 1 << 2,
    //     RHIBufferUsageRawBuffer = 1 << 3,
    //     RHIBufferUsageUnorderedAccess = 1 << 4,
    // };
    // using RHIBufferUsageFlags = uint32_t;

    // enum RHITextureUsageBit
    // {
    //     RHITextureUsageRenderTarget = 1 << 0,
    //     RHITextureUsageDepthStencil = 1 << 1,
    //     RHITextureUsageUnorderedAccess = 1 << 2,
    // };
    // using RHITextureUsageFlags = uint32_t;

    // struct RHIBufferInfo
    // {
    //     uint32_t            Stride = 1;
    //     uint32_t            Size = 1;
    //     RHIFormat           Format = RHIFormat::Unknown;
    //     RHIMemoryType       MemoryType = RHIMemoryType::GPUOnly;
    //     RHIAllocationType   AllocType = RHIAllocationType::Placed;
    //     RHIBufferUsageFlags Usage = 0;
    //     RHIHeap*            Heap = nullptr;
    //     uint32_t            HeapOffset = 0;
    // };

    // inline bool operator==(const RHIBufferInfo &lhs, const RHIBufferInfo &rhs)
    // {
    //     return lhs.Stride == rhs.Stride &&
    //            lhs.Size == rhs.Size &&
    //            lhs.Format == rhs.Format &&
    //            lhs.MemoryType == rhs.MemoryType &&
    //            lhs.AllocType == rhs.AllocType &&
    //            lhs.Usage == rhs.Usage;
    // }

    // struct RHITextureInfo
    // {
    //     uint32_t Width = 1;
    //     uint32_t Height = 1;
    //     uint32_t Depth = 1;
    //     uint32_t MipLevels = 1;
    //     uint32_t ArrayLayers = 1;
    //     RHITextureType Type = RHITextureType::Texture2D;
    //     RHIFormat Format = RHIFormat::Unknown;
    //     RHIMemoryType MemoryType = RHIMemoryType::GPUOnly;
    //     RHIAllocationType AllocType = RHIAllocationType::Placed;
    //     RHITextureUsageFlags Usage = 0;
    // };

    // struct RHIDeviceInfo
    // {
    //     RHIRenderBackend Backend = RHIRenderBackend::Vulkan;
    //     uint32_t MaxFrameLag = 3;
    // };

    // struct RHISwapchainInfo
    // {
    //     void* WindowHandle = nullptr;
    //     uint32_t Width = 1;
    //     uint32_t Height = 1;
    //     uint32_t BufferCount = 1;
    //     RHIFormat BufferFormat = RHIFormat::RGBA8SRGB;
    // };

    // enum class RHIPipelineType
    // {
    //     Graphics,
    //     Compute,
    //     Count
    // };
}

namespace std 
{
    template <typename T>
    struct hash<Vultana::RHIFlags<T>>
    {
        size_t operator()(Vultana::RHIFlags<T> flags) const
        {
            return hash<T>()(flags.Value());
        }
    };
}