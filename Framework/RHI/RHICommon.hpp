#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <tcb/span.hpp>

namespace RHI
{
    class RHIBuffer;
    class RHIHeap;
    class RHITexture;
    class RHIShader;

    static const uint32_t RHI_MAX_INFLIGHT_FRAMES = 3;
    static const uint32_t RHI_MAX_ROOT_CONSTANTS = 8;
    static const uint32_t RHI_MAX_COLOR_ATTACHMENT_COUNT = 8;
    static const uint32_t RHI_MAX_CBV_BINDING = 3;
    static const uint32_t RHI_MAX_RESOURCE_DESCRIPTOR_COUNT = 65536;
    static const uint32_t RHI_MAX_SAMPLER_DESCRIPTOR_COUNT = 128;
    static const uint32_t RHI_MAX_BUFFER_SIZE = 1024 * 1024 * 64; // 64MB

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

    enum class ERHIAllocationType
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
        RHITextureUsageRenderTarget     = 1 << 0,
        RHITextureUsageDepthStencil     = 1 << 1,
        RHITextureUsageUnorderedAccess  = 1 << 2,
        RHITextureUsageShared           = 1 << 3,
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
    };

    struct RHISwapchainDesc
    {
        void* WindowHandle = nullptr;
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint32_t BufferCount = RHI_MAX_INFLIGHT_FRAMES;
        ERHIFormat ColorFormat = ERHIFormat::RGBA8SRGB;
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
        ERHIAllocationType AllocationType = ERHIAllocationType::Placed;
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

    struct RHITextureDesc
    {
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint32_t Depth = 1;
        uint32_t MipLevels = 1;
        uint32_t ArraySize = 1;
        ERHITextureType Type = ERHITextureType::Texture2D;
        ERHIFormat Format = ERHIFormat::Unknown;
        ERHIMemoryType MemoryType = ERHIMemoryType::GPUOnly;
        ERHIAllocationType AllocationType = ERHIAllocationType::Placed;
        ERHITextureUsageFlags Usage = 0;
        RHIHeap* Heap = nullptr;
        uint32_t HeapOffset = 0;
    };
    inline bool operator==(const RHITextureDesc& lhs, const RHITextureDesc& rhs)
    {
        return lhs.Width == rhs.Width &&
            lhs.Height == rhs.Height &&
            lhs.Depth == rhs.Depth &&
            lhs.MipLevels == rhs.MipLevels &&
            lhs.ArraySize == rhs.ArraySize &&
            lhs.Type == rhs.Type &&
            lhs.Format == rhs.Format &&
            lhs.MemoryType == rhs.MemoryType &&
            lhs.AllocationType == rhs.AllocationType &&
            lhs.Usage == rhs.Usage;
    }

    struct RHIConstantBufferViewDesc
    {
        uint32_t Size = 0;
        uint32_t Offset = 0;
    };

    struct RHIShaderResourceViewDesc
    {
        ERHIShaderResourceViewType Type = ERHIShaderResourceViewType::Textue2D;
        ERHIFormat Format = ERHIFormat::Unknown;

        union
        {
            struct
            {
                uint32_t MipSlice = 0;
                uint32_t MipLevels = uint32_t(-1);
                uint32_t ArraySlice = 0;
                uint32_t ArraySize = 1;
                uint32_t PlaneSlice = 0;
            } Texture;

            struct
            {
                uint32_t Size = 0;
                uint32_t Offset = 0;
            } Buffer;
        };
        RHIShaderResourceViewDesc() : Texture() {}
    };
    inline bool operator==(const RHIShaderResourceViewDesc& lhs, const RHIShaderResourceViewDesc& rhs)
    {
        return lhs.Type == rhs.Type &&
            lhs.Texture.MipSlice == rhs.Texture.MipSlice &&
            lhs.Texture.MipLevels == rhs.Texture.MipLevels &&
            lhs.Texture.ArraySlice == rhs.Texture.ArraySlice &&
            lhs.Texture.ArraySize == rhs.Texture.ArraySize &&
            lhs.Texture.PlaneSlice == rhs.Texture.PlaneSlice;
    }

    struct RHIUnorderedAccessViewDesc
    {
        ERHIUnorderedAccessViewType Type = ERHIUnorderedAccessViewType::Texture2D;
        ERHIFormat Format = ERHIFormat::Unknown;

        union
        {
            struct
            {
                uint32_t MipSlice = 0;
                uint32_t ArraySlice = 0;
                uint32_t ArraySize = 1;
                uint32_t PlaneSlice = 0;
            } Texture;
            struct
            {
                uint32_t Size = 0;
                uint32_t Offset = 0;
            } Buffer;
        };
        RHIUnorderedAccessViewDesc() : Texture() {}
    };
    inline bool operator==(const RHIUnorderedAccessViewDesc& lhs, const RHIUnorderedAccessViewDesc& rhs)
    {
        return lhs.Type == rhs.Type &&
            lhs.Texture.MipSlice == rhs.Texture.MipSlice &&
            lhs.Texture.ArraySlice == rhs.Texture.ArraySlice &&
            lhs.Texture.ArraySize == rhs.Texture.ArraySize &&
            lhs.Texture.PlaneSlice == rhs.Texture.PlaneSlice;
    }

    struct RHIRenderPassColorAttachment
    {
        RHITexture* Texture = nullptr;
        uint32_t MipSlice = 0;
        uint32_t ArraySlice = 0;
        ERHIRenderPassLoadOp LoadOp = ERHIRenderPassLoadOp::Load;
        ERHIRenderPassStoreOp StoreOp = ERHIRenderPassStoreOp::Store;
        float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    struct RHIRenderPassDepthAttachment
    {
        RHITexture* Texture = nullptr;
        uint32_t MipSlice = 0;
        uint32_t ArraySlice = 0;
        ERHIRenderPassLoadOp DepthLoadOp = ERHIRenderPassLoadOp::Load;
        ERHIRenderPassStoreOp DepthStoreOp = ERHIRenderPassStoreOp::Store;
        ERHIRenderPassLoadOp StencilLoadOp = ERHIRenderPassLoadOp::Load;
        ERHIRenderPassStoreOp StencilStoreOp = ERHIRenderPassStoreOp::Store;
        float ClearDepth = 0.0f;
        uint32_t ClearStencil = 0;
        bool bReadOnly = false;
    };

    struct RHIRenderPassDesc
    {
        RHIRenderPassColorAttachment Color[RHI_MAX_COLOR_ATTACHMENT_COUNT];
        RHIRenderPassDepthAttachment Depth;
    };

    enum class ERHIShaderType
    {
        AS,
        MS,
        VS,
        PS,
        CS,
        Count
    };

    enum ERHIShaderCompileFlagBits
    {
        RHIShaderCompileFlagO0 = 1 << 0,
        RHIShaderCompileFlagO1 = 1 << 1,
        RHIShaderCompileFlagO2 = 1 << 2,
        RHIShaderCompileFlagO3 = 1 << 3,
    };
    using ERHIShaderCompileFlags = uint32_t;

    struct RHIShaderDesc
    {
        ERHIShaderType Type;
        std::string File;
        std::string EntryPoint;
        std::vector<std::string> Defines;
        ERHIShaderCompileFlags CompileFlags = 0;
    };

    enum class ERHICullMode
    {
        None,
        Front,
        Back,
        Count,
    };

    enum class RHICompareFunc
    {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
        Count,
    };
    
    enum class ERHIStencilOp
    {
        Keep,
        Zero,
        Replace,
        IncreaseClamp,
        DecreaseClamp,
        Invert,
        IncreaseWrap,
        DecreaseWrap,
        Count,
    };

    enum class ERHIBlendFactor
    {
        Zero,
        One,
        SrcColor,
        InvSrcColor,
        SrcAlpha,
        InvSrcAlpha,
        DestAlpha,
        InvDestAlpha,
        DestColor,
        InvDestColor,
        SrcAlphaClamp,
        ConstantFactor,
        InvConstantFactor,
        Count,
    };

    enum class ERHIBlendOp
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
        Count,
    };

    enum ERHIColorWriteMaskBits
    {
        RHIColorWriteMaskR = 1 << 0,
        RHIColorWriteMaskG = 1 << 1,
        RHIColorWriteMaskB = 1 << 2,
        RHIColorWriteMaskA = 1 << 3,

        RHIColorWriteMaskAll = RHIColorWriteMaskR | RHIColorWriteMaskG | RHIColorWriteMaskB | RHIColorWriteMaskA,
    };
    using ERHIColorWriteMask = uint8_t;
    
    enum class ERHIPrimitiveType
    {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        Count,
    };
    
    enum class ERHIPipelineType
    {
        Graphics,
        Compute,
        MeshShading,
        Count,
    };

    #pragma pack(push, 1)
    struct RHIRasterizerState
    {
        ERHICullMode CullMode = ERHICullMode::None;
        float DepthBias = 0.0f;
        float DepthBiasClamp = 0.0f;
        float DepthSlopeScale = 0.0f;
        float lineWidth = 1.0f;
        bool bWireFrame = false;
        bool bFrontCCW = false;
        bool bDepthClip = true;
    };

    struct RHIDepthStencilOp
    {
        ERHIStencilOp StencilFailOp = ERHIStencilOp::Keep;
        ERHIStencilOp DepthFailOp = ERHIStencilOp::Keep;
        ERHIStencilOp DepthStencilPassOp = ERHIStencilOp::Keep;
        RHICompareFunc StencilFunc = RHICompareFunc::Always;
    };

    struct RHIDepthStencilState
    {
        RHICompareFunc DepthFunc = RHICompareFunc::Always;
        bool bDepthTest = false;
        bool bDepthWrite = true;
        RHIDepthStencilOp FrontFace;
        RHIDepthStencilOp BackFace;
        bool bStencilEnable = false;
        uint8_t StencilReadMask = 0xFF;
        uint8_t StencilWriteMask = 0xFF;
    };

    struct RHIBlendState
    {
        bool bBlendEnable = false;
        ERHIBlendFactor ColorSrc = ERHIBlendFactor::One;
        ERHIBlendFactor ColorDst = ERHIBlendFactor::Zero;
        ERHIBlendOp ColorOp = ERHIBlendOp::Add;
        ERHIBlendFactor AlphaSrc = ERHIBlendFactor::One;
        ERHIBlendFactor AlphaDst = ERHIBlendFactor::Zero;
        ERHIBlendOp AlphaOp = ERHIBlendOp::Add;
        ERHIColorWriteMask WriteMask = RHIColorWriteMaskAll;
    };
    
    struct RHIGraphicsPipelineStateDesc
    {
        RHIShader* VS = nullptr;
        RHIShader* PS = nullptr;
        RHIRasterizerState RasterizerState;
        RHIDepthStencilState DepthStencilState;
        RHIBlendState BlendState[RHI_MAX_COLOR_ATTACHMENT_COUNT];
        ERHIFormat RTFormats[RHI_MAX_COLOR_ATTACHMENT_COUNT] = { ERHIFormat::Unknown };
        ERHIFormat DepthStencilFormat = ERHIFormat::Unknown;
        ERHIPrimitiveType PrimitiveType = ERHIPrimitiveType::TriangleList;
    };

    struct RHIComputePipelineStateDesc
    {
        RHIShader* CS = nullptr;
    };
    #pragma pack(pop)

    enum class ERHIFilter
    {
        Point,
        Linear,
    };

    enum class ERHISamplerAddressMode
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder,
    };

    enum class ERHISamplerReductionMode
    {
        Standard,
        Compare,
        Min,
        Max,
    };

    struct RHISamplerDesc
    {
        ERHIFilter MinFilter = ERHIFilter::Point;
        ERHIFilter MagFilter = ERHIFilter::Point;
        ERHIFilter MipFilter = ERHIFilter::Point;
        ERHISamplerReductionMode ReductionMode = ERHISamplerReductionMode::Standard;
        ERHISamplerAddressMode AddressU = ERHISamplerAddressMode::Repeat;
        ERHISamplerAddressMode AddressV = ERHISamplerAddressMode::Repeat;
        ERHISamplerAddressMode AddressW = ERHISamplerAddressMode::Repeat;
        RHICompareFunc CompareFunc = RHICompareFunc::Always;
        bool bEnableAnisotropy = false;
        float MaxAnisotropy = 1.0f;
        float MipLODBias = 0.0f;
        float MinLOD = 0.0f;
        float MaxLOD = 0.0f;
        float BorderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    struct RHIDrawCommand
    {
        uint32_t VertexCount = 0;
        uint32_t InstanceCount = 0;
        uint32_t FirstVertex = 0;
        uint32_t FirstInstance = 0;
    };

    struct RHIDrawIndexedCommand
    {
        uint32_t IndexCount = 0;
        uint32_t InstanceCount = 0;
        uint32_t FirstIndex = 0;
        uint32_t BaseVertex = 0;
        uint32_t FirstInstance = 0;
    };

    struct RHIDispatchCommand
    {
        uint32_t GroupCountX = 0;
        uint32_t GroupCountY = 0;
        uint32_t GroupCountZ = 0;
    };
}