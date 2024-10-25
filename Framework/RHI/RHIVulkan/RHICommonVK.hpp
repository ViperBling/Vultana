#pragma once
// #define NOMINMAX

#include "RHI/RHICommon.hpp"

// #define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

#include <unordered_map>
#include <cassert>

namespace RHI
{
    template<typename T>
    inline void SetDebugName(vk::Device device, vk::ObjectType objectType, T object, const char* name, vk::DispatchLoaderDynamic& dynamicLoader)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo;
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = reinterpret_cast<uint64_t&>(object);
        nameInfo.pObjectName = name;

        device.setDebugUtilsObjectNameEXT(nameInfo, dynamicLoader);
    }

    inline vk::Format ToVulkanFormat(ERHIFormat format, bool SRVOrRTV = false)
    {
        switch (format)
        {
        case ERHIFormat::Unknown:
            return vk::Format::eUndefined;
        case ERHIFormat::RGBA32F:
            return vk::Format::eR32G32B32A32Sfloat;
        case ERHIFormat::RGBA32UI:
            return vk::Format::eR32G32B32A32Uint;
        case ERHIFormat::RGBA32SI:
            return vk::Format::eR32G32B32A32Sint;
        case ERHIFormat::RGBA16F:
            return vk::Format::eR16G16B16A16Sfloat;
        case ERHIFormat::RGBA16UI:
            return vk::Format::eR16G16B16A16Uint;
        case ERHIFormat::RGBA16SI:
            return vk::Format::eR16G16B16A16Sint;
        case ERHIFormat::RGBA16UNORM:
            return vk::Format::eR16G16B16A16Unorm;
        case ERHIFormat::RGBA16SNORM:
            return vk::Format::eR16G16B16A16Snorm;
        case ERHIFormat::RGBA8UI:
            return vk::Format::eR8G8B8A8Uint;
        case ERHIFormat::RGBA8SI:
            return vk::Format::eR8G8B8A8Sint;
        case ERHIFormat::RGBA8UNORM:
            return vk::Format::eR8G8B8A8Unorm;
        case ERHIFormat::RGBA8SNORM:
            return vk::Format::eR8G8B8A8Snorm;
        case ERHIFormat::RGBA8SRGB:
            return SRVOrRTV ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
        case ERHIFormat::BGRA8UNORM:
            return vk::Format::eB8G8R8A8Unorm;
        case ERHIFormat::BGRA8SRGB:
            return SRVOrRTV ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;
        case ERHIFormat::RGB10A2UI:
            return vk::Format::eA2R10G10B10UintPack32;
        case ERHIFormat::RGB10A2UNORM:
            return vk::Format::eA2R10G10B10UnormPack32;
        case ERHIFormat::RGB32F:
            return vk::Format::eR32G32B32Sfloat;
        case ERHIFormat::RGB32UI:
            return vk::Format::eR32G32B32Uint;
        case ERHIFormat::RGB32SI:
            return vk::Format::eR32G32B32Sint;
        case ERHIFormat::R11G11B10F:
            return vk::Format::eB10G11R11UfloatPack32;
        case ERHIFormat::RGB9E5:
            return vk::Format::eE5B9G9R9UfloatPack32;
        case ERHIFormat::RG32F:
            return vk::Format::eR32G32Sfloat;
        case ERHIFormat::RG32UI:
            return vk::Format::eR32G32Uint;
        case ERHIFormat::RG32SI:
            return vk::Format::eR32G32Sint;
        case ERHIFormat::RG16F:
            return vk::Format::eR16G16Sfloat;
        case ERHIFormat::RG16UI:
            return vk::Format::eR16G16Uint;
        case ERHIFormat::RG16SI:
            return vk::Format::eR16G16Sint;
        case ERHIFormat::RG16UNORM:
            return vk::Format::eR16G16Unorm;
        case ERHIFormat::RG16SNORM:
            return vk::Format::eR16G16Snorm;
        case ERHIFormat::RG8UI:
            return vk::Format::eR8G8Uint;
        case ERHIFormat::RG8SI:
            return vk::Format::eR8G8Sint;
        case ERHIFormat::RG8UNORM:
            return vk::Format::eR8G8Unorm;
        case ERHIFormat::RG8SNORM:
            return vk::Format::eR8G8Snorm;
        case ERHIFormat::R32F:
            return vk::Format::eR32Sfloat;
        case ERHIFormat::R32UI:
            return vk::Format::eR32Uint;
        case ERHIFormat::R32SI:
            return vk::Format::eR32Sint;
        case ERHIFormat::R16F:
            return vk::Format::eR16Sfloat;
        case ERHIFormat::R16UI:
            return vk::Format::eR16Uint;
        case ERHIFormat::R16SI:
            return vk::Format::eR16Sint;
        case ERHIFormat::R16UNORM:
            return vk::Format::eR16Unorm;
        case ERHIFormat::R16SNORM:
            return vk::Format::eR16Snorm;
        case ERHIFormat::R8UI:
            return vk::Format::eR8Uint;
        case ERHIFormat::R8SI:
            return vk::Format::eR8Sint;
        case ERHIFormat::R8UNORM:
            return vk::Format::eR8Unorm;
        case ERHIFormat::R8SNORM:
            return vk::Format::eR8Snorm;
        case ERHIFormat::D32F:
            return vk::Format::eD32Sfloat;
        case ERHIFormat::D32FS8:
            return vk::Format::eD32SfloatS8Uint;
        case ERHIFormat::D16:
            return vk::Format::eD16Unorm;
        case ERHIFormat::BC1UNORM:
            return vk::Format::eBc1RgbUnormBlock;
        case ERHIFormat::BC1SRGB:
            return vk::Format::eBc1RgbSrgbBlock;
        case ERHIFormat::BC2UNORM:
            return vk::Format::eBc2UnormBlock;
        case ERHIFormat::BC2SRGB:
            return vk::Format::eBc2SrgbBlock;
        case ERHIFormat::BC3UNORM:
            return vk::Format::eBc3UnormBlock;
        case ERHIFormat::BC3SRGB:
            return vk::Format::eBc3SrgbBlock;
        case ERHIFormat::BC4UNORM:
            return vk::Format::eBc4UnormBlock;
        case ERHIFormat::BC4SNORM:
            return vk::Format::eBc4SnormBlock;
        case ERHIFormat::BC5UNORM:
            return vk::Format::eBc5UnormBlock;
        case ERHIFormat::BC5SNORM:
            return vk::Format::eBc5SnormBlock;
        case ERHIFormat::BC6U16F:
            return vk::Format::eBc6HUfloatBlock;
        case ERHIFormat::BC6S16F:
            return vk::Format::eBc6HSfloatBlock;
        case ERHIFormat::BC7UNORM:
            return vk::Format::eBc7UnormBlock;
        case ERHIFormat::BC7SRGB:
            return vk::Format::eBc7SrgbBlock;
        default:
            return vk::Format::eUndefined;
        }
    }

    inline VmaMemoryUsage ToVmaUsage(ERHIMemoryType type)
    {
        switch (type)
        {
        case ERHIMemoryType::GPUOnly:
            return VMA_MEMORY_USAGE_GPU_ONLY;
        case ERHIMemoryType::CPUOnly:
            return VMA_MEMORY_USAGE_CPU_ONLY;
        case ERHIMemoryType::CPUToGPU:
            return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case ERHIMemoryType::GPUToCPU:
            return VMA_MEMORY_USAGE_GPU_TO_CPU;
        default:
            return VMA_MEMORY_USAGE_AUTO;
        }
    }

    inline vk::ImageCreateInfo ToVulkanImageCreateInfo(const RHITextureDesc& desc)
    {
        vk::ImageCreateInfo createInfo;
        createInfo.imageType = desc.Type == ERHITextureType::Texture3D ? vk::ImageType::e3D : vk::ImageType::e2D;
        createInfo.format = ToVulkanFormat(desc.Format);
        createInfo.extent.width = desc.Width;
        createInfo.extent.height = desc.Height;
        createInfo.extent.depth = desc.Depth;
        createInfo.mipLevels = desc.MipLevels;
        createInfo.arrayLayers = desc.ArraySize;
        createInfo.samples = vk::SampleCountFlagBits::e1;
        createInfo.tiling = vk::ImageTiling::eOptimal;
        createInfo.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
        createInfo.sharingMode = vk::SharingMode::eExclusive;
        createInfo.initialLayout = vk::ImageLayout::eUndefined;

        if (desc.Usage & RHITextureUsageRenderTarget)
        {
            createInfo.usage |= vk::ImageUsageFlagBits::eColorAttachment;
        }

        if (desc.Usage & RHITextureUsageDepthStencil)
        {
            createInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        }

        if (desc.Usage & RHITextureUsageUnorderedAccess)
        {
            createInfo.usage |= vk::ImageUsageFlagBits::eStorage;
        }

        if (desc.Type == ERHITextureType::TextureCube || desc.Type == ERHITextureType::TextureCubeArray)
        {
            assert(desc.ArraySize % 6 == 0);
            createInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
        }

        if (desc.AllocationType == ERHIAllocationType::Sparse)
        {
            createInfo.flags |= vk::ImageCreateFlagBits::eSparseBinding | vk::ImageCreateFlagBits::eSparseResidency;
        }

        createInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat;

        return createInfo;
    }

    inline vk::ImageAspectFlags GetAspectFlags(ERHIFormat format)
    {
        vk::ImageAspectFlags aspectMask;

        if (format == ERHIFormat::D32FS8)
        {
            aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        }
        else if (format == ERHIFormat::D32F || format == ERHIFormat::D16)
        {
            aspectMask = vk::ImageAspectFlagBits::eDepth;
        }
        else
        {
            aspectMask = vk::ImageAspectFlagBits::eColor;
        }

        return aspectMask;
    }

    inline vk::PipelineStageFlags2 GetStageMask(ERHIAccessFlags flags)
    {
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eNone;

        if (flags & RHIAccessPresent)         stage |= vk::PipelineStageFlagBits2::eTopOfPipe;
        if (flags & RHIAccessRTV)             stage |= vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        if (flags & RHIAccessMaskDSV)         stage |= vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        if (flags & RHIAccessMaskVS)          stage |= /* vk::PipelineStageFlagBits2::eTaskShaderEXT | vk::PipelineStageFlagBits2::eMeshShaderEXT | */ vk::PipelineStageFlagBits2::eVertexShader;
        if (flags & RHIAccessMaskPS)          stage |= vk::PipelineStageFlagBits2::eFragmentShader;
        if (flags & RHIAccessMaskCS)          stage |= vk::PipelineStageFlagBits2::eComputeShader;
        if (flags & RHIAccessMaskCopy)        stage |= vk::PipelineStageFlagBits2::eCopy;
        if (flags & RHIAccessClearUAV)        stage |= vk::PipelineStageFlagBits2::eComputeShader;
        if (flags & RHIAccessShadingRate)     stage |= vk::PipelineStageFlagBits2::eFragmentShadingRateAttachmentKHR;
        if (flags & RHIAccessIndexBuffer)     stage |= vk::PipelineStageFlagBits2::eIndexInput;
        if (flags & RHIAccessIndirectArgs)    stage |= vk::PipelineStageFlagBits2::eDrawIndirect;
        // if (flags & RHIAccessMaskAS)          stage |= vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR;

        return stage;
    }

    inline vk::AccessFlags2 GetAccessMask(ERHIAccessFlags flags)
    {
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;

        if (flags & RHIAccessDiscard)
        {
            return access;
        }

        if (flags & RHIAccessRTV)             access |= vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
        if (flags & RHIAccessDSV)             access |= vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        if (flags & RHIAccessDSVReadOnly)     access |= vk::AccessFlagBits2::eDepthStencilAttachmentRead;
        if (flags & RHIAccessMaskSRV)         access |= vk::AccessFlagBits2::eShaderSampledRead | vk::AccessFlagBits2::eShaderStorageRead;
        if (flags & RHIAccessMaskUAV)         access |= vk::AccessFlagBits2::eShaderStorageWrite;
        if (flags & RHIAccessClearUAV)        access |= vk::AccessFlagBits2::eShaderStorageWrite;
        if (flags & RHIAccessCopyDst)         access |= vk::AccessFlagBits2::eTransferWrite;
        if (flags & RHIAccessCopySrc)         access |= vk::AccessFlagBits2::eTransferRead;
        if (flags & RHIAccessShadingRate)     access |= vk::AccessFlagBits2::eFragmentShadingRateAttachmentReadKHR;
        if (flags & RHIAccessIndexBuffer)     access |= vk::AccessFlagBits2::eIndexRead;
        if (flags & RHIAccessIndirectArgs)    access |= vk::AccessFlagBits2::eIndirectCommandRead;
        if (flags & RHIAccessASRead)          access |= vk::AccessFlagBits2::eAccelerationStructureReadKHR;
        if (flags & RHIAccessASWrite)         access |= vk::AccessFlagBits2::eAccelerationStructureWriteKHR;

        return access;
    }

    inline vk::ImageLayout GetImageLayout(ERHIAccessFlags flags)
    {
        if (flags & RHIAccessDiscard)         return vk::ImageLayout::eUndefined;
        if (flags & RHIAccessPresent)         return vk::ImageLayout::ePresentSrcKHR;
        if (flags & RHIAccessRTV)             return vk::ImageLayout::eColorAttachmentOptimal;
        if (flags & RHIAccessDSV)             return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        if (flags & RHIAccessDSVReadOnly)     return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        if (flags & RHIAccessMaskSRV)         return vk::ImageLayout::eShaderReadOnlyOptimal;
        if (flags & RHIAccessMaskUAV)         return vk::ImageLayout::eGeneral;
        if (flags & RHIAccessClearUAV)        return vk::ImageLayout::eGeneral;
        if (flags & RHIAccessCopyDst)         return vk::ImageLayout::eTransferDstOptimal;
        if (flags & RHIAccessCopySrc)         return vk::ImageLayout::eTransferSrcOptimal;
        if (flags & RHIAccessShadingRate)     return vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR;

        assert(false);
        return vk::ImageLayout::eUndefined;
    }

    inline vk::AttachmentLoadOp GetLoadOp(ERHIRenderPassLoadOp loadOp)
    {
        switch (loadOp)
        {
        case ERHIRenderPassLoadOp::Load:
            return vk::AttachmentLoadOp::eLoad;
        case ERHIRenderPassLoadOp::Clear:
            return vk::AttachmentLoadOp::eClear;
        case ERHIRenderPassLoadOp::DontCare:
            return vk::AttachmentLoadOp::eDontCare;
        default:
            return vk::AttachmentLoadOp::eLoad;
        }
    }

    inline vk::AttachmentStoreOp GetStoreOp(ERHIRenderPassStoreOp storeOp)
    {
        switch (storeOp)
        {
        case ERHIRenderPassStoreOp::Store:
            return vk::AttachmentStoreOp::eStore;
        case ERHIRenderPassStoreOp::DontCare:
            return vk::AttachmentStoreOp::eDontCare;
        default:
            return vk::AttachmentStoreOp::eStore;
        }
    }

    inline vk::PrimitiveTopology ToVKPrimitiveTopology(ERHIPrimitiveType type)
    {
        switch (type)
        {
        case ERHIPrimitiveType::PointList:
            return vk::PrimitiveTopology::ePointList;
        case ERHIPrimitiveType::LineList:
            return vk::PrimitiveTopology::eLineList;
        case ERHIPrimitiveType::LineStrip:
            return vk::PrimitiveTopology::eLineStrip;
        case ERHIPrimitiveType::TriangleList:
            return vk::PrimitiveTopology::eTriangleList;
        case ERHIPrimitiveType::TriangleStrip:
            return vk::PrimitiveTopology::eTriangleStrip;
        default:
            return vk::PrimitiveTopology::eTriangleList;
        }
    }

    inline vk::CullModeFlags ToVKCullMode(ERHICullMode mode)
    {
        switch (mode)
        {
        case ERHICullMode::None:
            return vk::CullModeFlagBits::eNone;
        case ERHICullMode::Front:
            return vk::CullModeFlagBits::eFront;
        case ERHICullMode::Back:
            return vk::CullModeFlagBits::eBack;
        default:
            return vk::CullModeFlagBits::eNone;
        }
    }

    inline vk::PipelineRasterizationStateCreateInfo ToVKPipelineRSStateCreateInfo(const RHIRasterizerState& state)
    {
        vk::PipelineRasterizationStateCreateInfo createInfo;
        createInfo.depthClampEnable = !state.bDepthClip;
        createInfo.polygonMode = state.bWireFrame ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
        createInfo.cullMode = ToVKCullMode(state.CullMode);
        createInfo.frontFace = state.bFrontCCW ? vk::FrontFace::eCounterClockwise : vk::FrontFace::eClockwise;
        createInfo.depthBiasEnable = state.DepthBias != 0.0f || state.DepthSlopeScale != 0.0f;
        createInfo.depthBiasConstantFactor = state.DepthBias;
        createInfo.depthBiasClamp = state.DepthBiasClamp;
        createInfo.depthBiasSlopeFactor = state.DepthSlopeScale;
        createInfo.lineWidth = state.bWireFrame ? state.lineWidth : 0.0f;

        return createInfo;
    }

    inline vk::CompareOp ToVKCompareOp(RHICompareFunc func)
    {
        switch (func)
        {
        case RHICompareFunc::Never:
            return vk::CompareOp::eNever;
        case RHICompareFunc::Less:
            return vk::CompareOp::eLess;
        case RHICompareFunc::Equal:
            return vk::CompareOp::eEqual;
        case RHICompareFunc::LessEqual:
            return vk::CompareOp::eLessOrEqual;
        case RHICompareFunc::Greater:
            return vk::CompareOp::eGreater;
        case RHICompareFunc::NotEqual:
            return vk::CompareOp::eNotEqual;
        case RHICompareFunc::GreaterEqual:
            return vk::CompareOp::eGreaterOrEqual;
        case RHICompareFunc::Always:
            return vk::CompareOp::eAlways;
        default:
            return vk::CompareOp::eAlways;
        }
    }

    inline vk::StencilOp ToVKStencilOp(ERHIStencilOp stencilOp)
    {
        switch (stencilOp)
        {
        case ERHIStencilOp::Keep:
            return vk::StencilOp::eKeep;
        case ERHIStencilOp::Zero:
            return vk::StencilOp::eZero;
        case ERHIStencilOp::Replace:
            return vk::StencilOp::eReplace;
        case ERHIStencilOp::IncreaseClamp:
            return vk::StencilOp::eIncrementAndClamp;
        case ERHIStencilOp::DecreaseClamp:
            return vk::StencilOp::eDecrementAndClamp;
        case ERHIStencilOp::Invert:
            return vk::StencilOp::eInvert;
        case ERHIStencilOp::IncreaseWrap:
            return vk::StencilOp::eIncrementAndWrap;
        case ERHIStencilOp::DecreaseWrap:
            return vk::StencilOp::eDecrementAndWrap;
        default:
            return vk::StencilOp::eKeep;
        }
    }

    inline vk::StencilOpState ToVKStencilOpState(RHIDepthStencilOp state, uint8_t readMask, uint8_t writeMask)
    {
        vk::StencilOpState stencilState;
        stencilState.failOp = ToVKStencilOp(state.StencilFailOp);
        stencilState.passOp = ToVKStencilOp(state.DepthStencilPassOp);
        stencilState.depthFailOp = ToVKStencilOp(state.DepthFailOp);
        stencilState.compareOp = ToVKCompareOp(state.StencilFunc);
        stencilState.compareMask = readMask;
        stencilState.writeMask = writeMask;

        return stencilState;
    }

    inline vk::PipelineDepthStencilStateCreateInfo ToVKPipelineDSStateCreateInfo(const RHIDepthStencilState& state)
    {
        vk::PipelineDepthStencilStateCreateInfo createInfo;
        createInfo.depthTestEnable = state.bDepthTest;
        createInfo.depthWriteEnable = state.bDepthWrite;
        createInfo.depthCompareOp = ToVKCompareOp(state.DepthFunc);
        createInfo.stencilTestEnable = state.bStencilEnable;
        createInfo.front = ToVKStencilOpState(state.FrontFace, state.StencilReadMask, state.StencilWriteMask);
        createInfo.back = ToVKStencilOpState(state.BackFace, state.StencilReadMask, state.StencilWriteMask);

        return createInfo;
    }

    inline vk::BlendFactor ToVKBlendFactor(ERHIBlendFactor factor, bool alpha = false)
    {
        switch (factor)
        {
        case ERHIBlendFactor::Zero:
            return vk::BlendFactor::eZero;
        case ERHIBlendFactor::One:
            return vk::BlendFactor::eOne;
        case ERHIBlendFactor::SrcColor:
            return vk::BlendFactor::eSrcColor;
        case ERHIBlendFactor::InvSrcColor:
            return vk::BlendFactor::eOneMinusSrcColor;
        case ERHIBlendFactor::SrcAlpha:
            return vk::BlendFactor::eSrcAlpha;
        case ERHIBlendFactor::InvSrcAlpha:
            return vk::BlendFactor::eOneMinusSrcAlpha;
        case ERHIBlendFactor::DestColor:
            return vk::BlendFactor::eDstColor;
        case ERHIBlendFactor::InvDestColor:
            return vk::BlendFactor::eOneMinusDstColor;
        case ERHIBlendFactor::DestAlpha:
            return vk::BlendFactor::eDstAlpha;
        case ERHIBlendFactor::InvDestAlpha:
            return vk::BlendFactor::eOneMinusDstAlpha;
        case ERHIBlendFactor::SrcAlphaClamp:
            return vk::BlendFactor::eSrcAlphaSaturate;
        case ERHIBlendFactor::ConstantFactor:
            return alpha ? vk::BlendFactor::eConstantAlpha : vk::BlendFactor::eConstantColor;
        case ERHIBlendFactor::InvConstantFactor:
            return alpha ? vk::BlendFactor::eOneMinusConstantAlpha : vk::BlendFactor::eOneMinusConstantColor;
        default:
            return vk::BlendFactor::eZero;
        }
    }

    inline vk::BlendOp ToVKBlendOp(ERHIBlendOp op)
    {
        switch (op)
        {
        case ERHIBlendOp::Add:
            return vk::BlendOp::eAdd;
        case ERHIBlendOp::Subtract:
            return vk::BlendOp::eSubtract;
        case ERHIBlendOp::ReverseSubtract:
            return vk::BlendOp::eReverseSubtract;
        case ERHIBlendOp::Min:
            return vk::BlendOp::eMin;
        case ERHIBlendOp::Max:
            return vk::BlendOp::eMax;
        default:
            return vk::BlendOp::eAdd;
        }
    }

    inline vk::PipelineColorBlendStateCreateInfo ToVKPipelineCBStateCreateInfo(const RHIBlendState* states, vk::PipelineColorBlendAttachmentState* vkStates)
    {
        for (uint32_t i = 0; i < RHI_MAX_COLOR_ATTACHMENT_COUNT; i++)
        {
            vkStates[i].blendEnable = states[i].bBlendEnable;
            vkStates[i].srcColorBlendFactor = ToVKBlendFactor(states[i].ColorSrc);
            vkStates[i].dstColorBlendFactor = ToVKBlendFactor(states[i].ColorDst);
            vkStates[i].colorBlendOp = ToVKBlendOp(states[i].ColorOp);
            vkStates[i].srcAlphaBlendFactor = ToVKBlendFactor(states[i].AlphaSrc, true);
            vkStates[i].dstAlphaBlendFactor = ToVKBlendFactor(states[i].AlphaDst, true);
            vkStates[i].alphaBlendOp = ToVKBlendOp(states[i].AlphaOp);
            vkStates[i].colorWriteMask = static_cast<vk::ColorComponentFlags>(states[i].WriteMask);
        }

        vk::PipelineColorBlendStateCreateInfo createInfo;
        createInfo.attachmentCount = RHI_MAX_COLOR_ATTACHMENT_COUNT;
        createInfo.pAttachments = vkStates;

        return createInfo;
    }

    template<typename T>
    inline vk::PipelineRenderingCreateInfo ToVKPipelineRenderingCreateInfo(const T& pipelineDesc, vk::Format* colorFormats)
    {
        for (uint32_t i = 0; i < RHI_MAX_COLOR_ATTACHMENT_COUNT; i++)
        {
            colorFormats[i] = ToVulkanFormat(pipelineDesc.RTFormats[i], true);
        }

        vk::PipelineRenderingCreateInfo renderingCI {};
        renderingCI.colorAttachmentCount = RHI_MAX_COLOR_ATTACHMENT_COUNT;
        renderingCI.pColorAttachmentFormats = colorFormats;
        renderingCI.depthAttachmentFormat = ToVulkanFormat(pipelineDesc.DepthStencilFormat);

        if (pipelineDesc.DepthStencilFormat == ERHIFormat::D32FS8)
        {
            renderingCI.stencilAttachmentFormat = renderingCI.depthAttachmentFormat;
        }
        return renderingCI;
    }

    inline vk::Filter ToVKFilter(ERHIFilter filter)
    {
        switch (filter)
        {
        case ERHIFilter::Point:
            return vk::Filter::eNearest;
        case ERHIFilter::Linear:
            return vk::Filter::eLinear;
        default:
            return vk::Filter::eNearest;
        }
    }

    inline vk::SamplerMipmapMode ToVKSamplerMipmapMode(ERHIFilter filter)
    {
        switch (filter)
        {
        case ERHIFilter::Point:
            return vk::SamplerMipmapMode::eNearest;
        case ERHIFilter::Linear:
            return vk::SamplerMipmapMode::eLinear;
        default:
            return vk::SamplerMipmapMode::eNearest;
        }
    }

    inline vk::SamplerAddressMode ToVKSamplerAddressMode(ERHISamplerAddressMode mode)
    {
        switch (mode)
        {
        case ERHISamplerAddressMode::Repeat:
            return vk::SamplerAddressMode::eRepeat;
        case ERHISamplerAddressMode::MirroredRepeat:
            return vk::SamplerAddressMode::eMirroredRepeat;
        case ERHISamplerAddressMode::ClampToEdge:
            return vk::SamplerAddressMode::eClampToEdge;
        case ERHISamplerAddressMode::ClampToBorder:
            return vk::SamplerAddressMode::eClampToBorder;
        default:
            return vk::SamplerAddressMode::eRepeat;
        }
    }

    inline vk::SamplerReductionMode ToVKSamplerReductionMode(ERHISamplerReductionMode mode)
    {
        switch (mode)
        {
        case ERHISamplerReductionMode::Standard:
            return vk::SamplerReductionMode::eWeightedAverage;
        case ERHISamplerReductionMode::Compare:
            return vk::SamplerReductionMode::eWeightedAverage;
        case ERHISamplerReductionMode::Min:
            return vk::SamplerReductionMode::eMin;
        case ERHISamplerReductionMode::Max:
            return vk::SamplerReductionMode::eMax;
        default:
            return vk::SamplerReductionMode::eWeightedAverage;
        }
    }
}