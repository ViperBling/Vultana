#pragma once

#include "Utilities/Math.hpp"
#include "RHI/RHICommon.hpp"

#include <unordered_map>
#include <cassert>
#include <vulkan/vulkan.hpp>

namespace RHI
{
    DECLARE_EC_FUNC()
    DECLARE_FC_FUNC()

    #define VK_KRONOS_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

    template <typename A, typename B>
    static std::unordered_map<A, B>& GetEnumMap()
    {
        static std::unordered_map<A, B> map;
        return map;
    }

    template <typename A, typename B>
    B VKEnumCast(const A& value)
    {
        auto& map = GetEnumMap<A, B>();
        auto it = map.find(value);
        assert((it != map.end()));
        return static_cast<B>(it->second);
    }

    ECIMPL_BEGIN(vk::PhysicalDeviceType, RHIDeviceType)
        ECIMPL_ITEM(vk::PhysicalDeviceType::eOther, RHIDeviceType::Software)
        ECIMPL_ITEM(vk::PhysicalDeviceType::eIntegratedGpu, RHIDeviceType::Software)
        ECIMPL_ITEM(vk::PhysicalDeviceType::eDiscreteGpu, RHIDeviceType::Hardware)
        ECIMPL_ITEM(vk::PhysicalDeviceType::eVirtualGpu, RHIDeviceType::Software)
        ECIMPL_ITEM(vk::PhysicalDeviceType::eCpu, RHIDeviceType::Software)
    ECIMPL_END(RHIDeviceType)

    ECIMPL_BEGIN(RHIFormat, vk::Format)
        ECIMPL_ITEM(RHIFormat::R8_UNORM, vk::Format::eR8Unorm)
        ECIMPL_ITEM(RHIFormat::R8_SNORM, vk::Format::eR8Snorm)
        ECIMPL_ITEM(RHIFormat::R8_UINT, vk::Format::eR8Uint)
        ECIMPL_ITEM(RHIFormat::R8_SINT, vk::Format::eR8Sint)
        ECIMPL_ITEM(RHIFormat::R16_UINT, vk::Format::eR16Uint)
        ECIMPL_ITEM(RHIFormat::R16_SINT, vk::Format::eR16Sint)
        ECIMPL_ITEM(RHIFormat::R16_FLOAT, vk::Format::eR16Sfloat)
        ECIMPL_ITEM(RHIFormat::RG8_UNORM, vk::Format::eR8G8Unorm)
        ECIMPL_ITEM(RHIFormat::RG8_SNORM, vk::Format::eR8G8Snorm)
        ECIMPL_ITEM(RHIFormat::RG8_UINT, vk::Format::eR8G8Uint)
        ECIMPL_ITEM(RHIFormat::RG8_SINT, vk::Format::eR8G8Sint)
        ECIMPL_ITEM(RHIFormat::R32_UINT, vk::Format::eR32Uint)
        ECIMPL_ITEM(RHIFormat::R32_SINT, vk::Format::eR32Sint)
        ECIMPL_ITEM(RHIFormat::R32_FLOAT, vk::Format::eR32Sfloat)
        ECIMPL_ITEM(RHIFormat::RG16_UINT, vk::Format::eR16G16Uint)
        ECIMPL_ITEM(RHIFormat::RG16_SINT, vk::Format::eR16G16Sint)
        ECIMPL_ITEM(RHIFormat::RG16_FLOAT, vk::Format::eR16G16Sfloat)
        ECIMPL_ITEM(RHIFormat::RGBA8_UNORM, vk::Format::eR8G8B8A8Unorm)
        ECIMPL_ITEM(RHIFormat::RGBA8_UNORM_SRGB, vk::Format::eR8G8B8A8Srgb)
        ECIMPL_ITEM(RHIFormat::RGBA8_SNORM, vk::Format::eR8G8B8A8Snorm)
        ECIMPL_ITEM(RHIFormat::RGBA8_UINT, vk::Format::eR8G8B8A8Uint)
        ECIMPL_ITEM(RHIFormat::RGBA8_SINT, vk::Format::eR8G8B8A8Sint)
        ECIMPL_ITEM(RHIFormat::BGRA8_UNORM, vk::Format::eB8G8R8A8Unorm)
        ECIMPL_ITEM(RHIFormat::BGRA8_UNORM_SRGB, vk::Format::eB8G8R8A8Srgb)
        ECIMPL_ITEM(RHIFormat::RGB9E5_FLOAT, vk::Format::eE5B9G9R9UfloatPack32)
        ECIMPL_ITEM(RHIFormat::RGB10A2_UNORM, vk::Format::eA2B10G10R10UnormPack32)
        ECIMPL_ITEM(RHIFormat::RG11B10_FLOAT, vk::Format::eB10G11R11UfloatPack32)
        ECIMPL_ITEM(RHIFormat::RG32_UINT, vk::Format::eR32G32Uint)
        ECIMPL_ITEM(RHIFormat::RG32_SINT, vk::Format::eR32G32Sint)
        ECIMPL_ITEM(RHIFormat::RG32_FLOAT, vk::Format::eR32G32Sfloat)
        ECIMPL_ITEM(RHIFormat::RGBA16_UINT, vk::Format::eR16G16B16A16Uint)
        ECIMPL_ITEM(RHIFormat::RGBA16_SINT, vk::Format::eR16G16B16A16Sint)
        ECIMPL_ITEM(RHIFormat::RGBA16_FLOAT, vk::Format::eR16G16B16A16Sfloat)
        ECIMPL_ITEM(RHIFormat::RGBA32_UINT, vk::Format::eR32G32B32A32Uint)
        ECIMPL_ITEM(RHIFormat::RGBA32_SINT, vk::Format::eR32G32B32A32Sint)
        ECIMPL_ITEM(RHIFormat::RGBA32_FLOAT, vk::Format::eR32G32B32A32Sfloat)
        ECIMPL_ITEM(RHIFormat::D16_UNORM, vk::Format::eD16Unorm)
        ECIMPL_ITEM(RHIFormat::D24_UNORM_S8_UINT, vk::Format::eD24UnormS8Uint)
        ECIMPL_ITEM(RHIFormat::D32_FLOAT, vk::Format::eD32Sfloat)
        ECIMPL_ITEM(RHIFormat::D32_FLOAT_S8_UINT, vk::Format::eD32SfloatS8Uint)
        ECIMPL_ITEM(RHIFormat::Count, vk::Format::eUndefined)
    ECIMPL_END(vk::Format)

    ECIMPL_BEGIN(RHICommandQueueType, vk::QueueFlagBits)
        ECIMPL_ITEM(RHICommandQueueType::Graphics, vk::QueueFlagBits::eGraphics)
        ECIMPL_ITEM(RHICommandQueueType::Compute, vk::QueueFlagBits::eCompute)
        ECIMPL_ITEM(RHICommandQueueType::Transfer, vk::QueueFlagBits::eTransfer)
    ECIMPL_END(vk::QueueFlagBits)

    ECIMPL_BEGIN(RHITextureDimension, vk::ImageType)
        ECIMPL_ITEM(RHITextureDimension::Texture1D, vk::ImageType::e1D)
        ECIMPL_ITEM(RHITextureDimension::Texture2D, vk::ImageType::e2D)
        ECIMPL_ITEM(RHITextureDimension::Texture3D, vk::ImageType::e3D)
    ECIMPL_END(vk::ImageType)

    ECIMPL_BEGIN(RHITextureViewDimension, vk::ImageViewType)
        ECIMPL_ITEM(RHITextureViewDimension::TextureView1D, vk::ImageViewType::e1D)
        ECIMPL_ITEM(RHITextureViewDimension::TextureView2D, vk::ImageViewType::e2D)
        ECIMPL_ITEM(RHITextureViewDimension::TextureView2DArray, vk::ImageViewType::e2DArray)
        ECIMPL_ITEM(RHITextureViewDimension::TextureViewCube, vk::ImageViewType::eCube)
        ECIMPL_ITEM(RHITextureViewDimension::TextureViewCubeArray, vk::ImageViewType::eCubeArray)
        ECIMPL_ITEM(RHITextureViewDimension::TextureView3D, vk::ImageViewType::e3D)
    ECIMPL_END(vk::ImageViewType)

    ECIMPL_BEGIN(RHITextureType, vk::ImageAspectFlags)
        ECIMPL_ITEM(RHITextureType::Color, vk::ImageAspectFlagBits::eColor)
        ECIMPL_ITEM(RHITextureType::Depth, vk::ImageAspectFlagBits::eDepth)
        ECIMPL_ITEM(RHITextureType::Stencil, vk::ImageAspectFlagBits::eStencil)
        ECIMPL_ITEM(RHITextureType::DepthStencil, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
    ECIMPL_END(vk::ImageAspectFlags)

    

    ECIMPL_BEGIN(RHIPrimitiveTopologyType, vk::PrimitiveTopology)
        ECIMPL_ITEM(RHIPrimitiveTopologyType::Point, vk::PrimitiveTopology::ePointList)
        ECIMPL_ITEM(RHIPrimitiveTopologyType::Line, vk::PrimitiveTopology::eLineList)
        ECIMPL_ITEM(RHIPrimitiveTopologyType::Triangle, vk::PrimitiveTopology::eTriangleList)
    ECIMPL_END(vk::PrimitiveTopology)

    ECIMPL_BEGIN(RHICullMode, vk::CullModeFlagBits)
        ECIMPL_ITEM(RHICullMode::None, vk::CullModeFlagBits::eNone)
        ECIMPL_ITEM(RHICullMode::Front, vk::CullModeFlagBits::eFront)
        ECIMPL_ITEM(RHICullMode::Back, vk::CullModeFlagBits::eBack)
    ECIMPL_END(vk::CullModeFlagBits)

    ECIMPL_BEGIN(RHIBlendOp, vk::BlendOp)
        ECIMPL_ITEM(RHIBlendOp::Add, vk::BlendOp::eAdd)
        ECIMPL_ITEM(RHIBlendOp::Subtract, vk::BlendOp::eSubtract)
        ECIMPL_ITEM(RHIBlendOp::ReverseSubtract, vk::BlendOp::eReverseSubtract)
        ECIMPL_ITEM(RHIBlendOp::Min, vk::BlendOp::eMin)
        ECIMPL_ITEM(RHIBlendOp::Max, vk::BlendOp::eMax)
    ECIMPL_END(vk::BlendOp)

    ECIMPL_BEGIN(RHIBlendFactor, vk::BlendFactor)
        ECIMPL_ITEM(RHIBlendFactor::Zero, vk::BlendFactor::eZero)
        ECIMPL_ITEM(RHIBlendFactor::One, vk::BlendFactor::eOne)
        ECIMPL_ITEM(RHIBlendFactor::Src, vk::BlendFactor::eSrcColor)
        ECIMPL_ITEM(RHIBlendFactor::OneMinusSrc, vk::BlendFactor::eOneMinusSrcColor)
        ECIMPL_ITEM(RHIBlendFactor::SrcAlpha, vk::BlendFactor::eSrcAlpha)
        ECIMPL_ITEM(RHIBlendFactor::OneMinusSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha)
        ECIMPL_ITEM(RHIBlendFactor::Dst, vk::BlendFactor::eDstColor)
        ECIMPL_ITEM(RHIBlendFactor::OneMinusDst, vk::BlendFactor::eOneMinusDstColor)
        ECIMPL_ITEM(RHIBlendFactor::DstAlpha, vk::BlendFactor::eDstAlpha)
        ECIMPL_ITEM(RHIBlendFactor::OneMinusDstAlpha, vk::BlendFactor::eOneMinusDstAlpha)
    ECIMPL_END(vk::BlendFactor)

    ECIMPL_BEGIN(RHIVertexFormat, vk::Format)
        ECIMPL_ITEM(RHIVertexFormat::UINT_8X2, vk::Format::eR8G8Uint)
        ECIMPL_ITEM(RHIVertexFormat::UINT_8X4, vk::Format::eR8G8B8A8Uint)
        ECIMPL_ITEM(RHIVertexFormat::SINT_8X2, vk::Format::eR8G8Sint)
        ECIMPL_ITEM(RHIVertexFormat::SINT_8X4, vk::Format::eR8G8B8A8Sint)
        ECIMPL_ITEM(RHIVertexFormat::UNORM_8X2, vk::Format::eR8G8Unorm)
        ECIMPL_ITEM(RHIVertexFormat::UNORM_8X4, vk::Format::eR8G8B8A8Unorm)
        ECIMPL_ITEM(RHIVertexFormat::SNORM_8X2, vk::Format::eR8G8Snorm)
        ECIMPL_ITEM(RHIVertexFormat::SNORM_8X4, vk::Format::eR8G8B8A8Snorm)
        ECIMPL_ITEM(RHIVertexFormat::UINT_16X2, vk::Format::eR16G16Uint)
        ECIMPL_ITEM(RHIVertexFormat::UINT_16X4, vk::Format::eR16G16B16A16Uint)
        ECIMPL_ITEM(RHIVertexFormat::SINT_16X2, vk::Format::eR16G16Sint)
        ECIMPL_ITEM(RHIVertexFormat::SINT_16X4, vk::Format::eR16G16B16A16Sint)
        ECIMPL_ITEM(RHIVertexFormat::UNORM_16X2, vk::Format::eR16G16Unorm)
        ECIMPL_ITEM(RHIVertexFormat::UNORM_16X4, vk::Format::eR16G16B16A16Unorm)
        ECIMPL_ITEM(RHIVertexFormat::SNORM_16X2, vk::Format::eR16G16Snorm)
        ECIMPL_ITEM(RHIVertexFormat::SNORM_16X4, vk::Format::eR16G16B16A16Snorm)
        ECIMPL_ITEM(RHIVertexFormat::FLOAT_16X2, vk::Format::eR16G16Sfloat)
        ECIMPL_ITEM(RHIVertexFormat::FLOAT_16X4, vk::Format::eR16G16B16A16Sfloat)
        ECIMPL_ITEM(RHIVertexFormat::FLOAT_32X1, vk::Format::eR32Sfloat)
        ECIMPL_ITEM(RHIVertexFormat::FLOAT_32X2, vk::Format::eR32G32Sfloat)
        ECIMPL_ITEM(RHIVertexFormat::FLOAT_32X3, vk::Format::eR32G32B32Sfloat)
        ECIMPL_ITEM(RHIVertexFormat::FLOAT_32X4, vk::Format::eR32G32B32A32Sfloat)
        ECIMPL_ITEM(RHIVertexFormat::UINT_32X1, vk::Format::eR32Uint)
        ECIMPL_ITEM(RHIVertexFormat::UINT_32X2, vk::Format::eR32G32Uint)
        ECIMPL_ITEM(RHIVertexFormat::UINT_32X3, vk::Format::eR32G32B32Uint)
        ECIMPL_ITEM(RHIVertexFormat::UINT_32X4, vk::Format::eR32G32B32A32Uint)
        ECIMPL_ITEM(RHIVertexFormat::SINT_32X1, vk::Format::eR32Sint)
        ECIMPL_ITEM(RHIVertexFormat::SINT_32X2, vk::Format::eR32G32Sint)
        ECIMPL_ITEM(RHIVertexFormat::SINT_32X3, vk::Format::eR32G32B32Sint)
        ECIMPL_ITEM(RHIVertexFormat::SINT_32X4, vk::Format::eR32G32B32A32Sint)
    ECIMPL_END(vk::Format)

    ECIMPL_BEGIN(RHIIndexFormat, vk::IndexType)
        ECIMPL_ITEM(RHIIndexFormat::UINT_16, vk::IndexType::eUint16)
        ECIMPL_ITEM(RHIIndexFormat::UINT_32, vk::IndexType::eUint32)
        ECIMPL_ITEM(RHIIndexFormat::Count, vk::IndexType::eNoneKHR)
    ECIMPL_END(vk::IndexType)

    ECIMPL_BEGIN(RHIBindingType, vk::DescriptorType)
        ECIMPL_ITEM(RHIBindingType::UniformBuffer, vk::DescriptorType::eUniformBuffer)
        ECIMPL_ITEM(RHIBindingType::StorageBuffer, vk::DescriptorType::eStorageBuffer)
        ECIMPL_ITEM(RHIBindingType::Sampler, vk::DescriptorType::eSampler)
        ECIMPL_ITEM(RHIBindingType::Texture, vk::DescriptorType::eSampledImage)
        ECIMPL_ITEM(RHIBindingType::StorageTexture, vk::DescriptorType::eStorageImage)
    ECIMPL_END(vk::DescriptorType)

    ECIMPL_BEGIN(RHISamplerAddressMode, vk::SamplerAddressMode)
        ECIMPL_ITEM(RHISamplerAddressMode::Clamp, vk::SamplerAddressMode::eClampToEdge)
        ECIMPL_ITEM(RHISamplerAddressMode::Repeat, vk::SamplerAddressMode::eRepeat)
        ECIMPL_ITEM(RHISamplerAddressMode::Mirror, vk::SamplerAddressMode::eMirroredRepeat)
    ECIMPL_END(vk::SamplerAddressMode)

    ECIMPL_BEGIN(RHISamplerFilterMode, vk::Filter)
        ECIMPL_ITEM(RHISamplerFilterMode::Nearest, vk::Filter::eNearest)
        ECIMPL_ITEM(RHISamplerFilterMode::Linear, vk::Filter::eLinear)
    ECIMPL_END(vk::Filter)

    ECIMPL_BEGIN(RHISamplerFilterMode, vk::SamplerMipmapMode)
        ECIMPL_ITEM(RHISamplerFilterMode::Nearest, vk::SamplerMipmapMode::eNearest)
        ECIMPL_ITEM(RHISamplerFilterMode::Linear, vk::SamplerMipmapMode::eLinear)
    ECIMPL_END(vk::SamplerMipmapMode)

    ECIMPL_BEGIN(RHICompareOp, vk::CompareOp)
        ECIMPL_ITEM(RHICompareOp::Never, vk::CompareOp::eNever)
        ECIMPL_ITEM(RHICompareOp::Less, vk::CompareOp::eLess)
        ECIMPL_ITEM(RHICompareOp::Equal, vk::CompareOp::eEqual)
        ECIMPL_ITEM(RHICompareOp::LessEqual, vk::CompareOp::eLessOrEqual)
        ECIMPL_ITEM(RHICompareOp::Greater, vk::CompareOp::eGreater)
        ECIMPL_ITEM(RHICompareOp::NotEqual, vk::CompareOp::eNotEqual)
        ECIMPL_ITEM(RHICompareOp::GreaterEqual, vk::CompareOp::eGreaterOrEqual)
        ECIMPL_ITEM(RHICompareOp::Always, vk::CompareOp::eAlways)
    ECIMPL_END(vk::CompareOp)

    ECIMPL_BEGIN(RHIStencilOp, vk::StencilOp)
        ECIMPL_ITEM(RHIStencilOp::Keep, vk::StencilOp::eKeep)
        ECIMPL_ITEM(RHIStencilOp::Zero, vk::StencilOp::eZero)
        ECIMPL_ITEM(RHIStencilOp::Replace, vk::StencilOp::eReplace)
        ECIMPL_ITEM(RHIStencilOp::IncrementClamp, vk::StencilOp::eIncrementAndClamp)
        ECIMPL_ITEM(RHIStencilOp::DecrementClamp, vk::StencilOp::eDecrementAndClamp)
        ECIMPL_ITEM(RHIStencilOp::IncrementWrap, vk::StencilOp::eIncrementAndWrap)
        ECIMPL_ITEM(RHIStencilOp::DecrementWrap, vk::StencilOp::eDecrementAndWrap)
    ECIMPL_END(vk::StencilOp)

    ECIMPL_BEGIN(RHILoadOp, vk::AttachmentLoadOp)
        ECIMPL_ITEM(RHILoadOp::Load, vk::AttachmentLoadOp::eLoad)
        ECIMPL_ITEM(RHILoadOp::Clear, vk::AttachmentLoadOp::eClear)
        ECIMPL_ITEM(RHILoadOp::Count, vk::AttachmentLoadOp::eNoneEXT)
    ECIMPL_END(vk::AttachmentLoadOp)

    ECIMPL_BEGIN(RHIStoreOp, vk::AttachmentStoreOp)
        ECIMPL_ITEM(RHIStoreOp::Store, vk::AttachmentStoreOp::eStore)
        ECIMPL_ITEM(RHIStoreOp::Discard, vk::AttachmentStoreOp::eDontCare)
        ECIMPL_ITEM(RHIStoreOp::Count, vk::AttachmentStoreOp::eNoneEXT)
    ECIMPL_END(vk::AttachmentStoreOp)

    ECIMPL_BEGIN(RHITextureState, vk::ImageLayout)
        ECIMPL_ITEM(RHITextureState::Undefined, vk::ImageLayout::eUndefined)
        ECIMPL_ITEM(RHITextureState::RenderTarget, vk::ImageLayout::eColorAttachmentOptimal)
        ECIMPL_ITEM(RHITextureState::Present, vk::ImageLayout::ePresentSrcKHR)
        ECIMPL_ITEM(RHITextureState::Count, vk::ImageLayout::eGeneral)
    ECIMPL_END(vk::ImageLayout)

    ECIMPL_BEGIN(RHIPresentMode, vk::PresentModeKHR)
        ECIMPL_ITEM(RHIPresentMode::Immediate, vk::PresentModeKHR::eImmediate)
        ECIMPL_ITEM(RHIPresentMode::Vsync, vk::PresentModeKHR::eFifo)
        ECIMPL_ITEM(RHIPresentMode::Count, vk::PresentModeKHR::eImmediate)
    ECIMPL_END(vk::PresentModeKHR)

    FCIMPL_BEGIN(RHIShaderStageFlags, vk::ShaderStageFlagBits)
        FCIMPL_ITEM(RHIShaderStageBits::Vertex, vk::ShaderStageFlagBits::eVertex)
        FCIMPL_ITEM(RHIShaderStageBits::Pixel, vk::ShaderStageFlagBits::eFragment)
        FCIMPL_ITEM(RHIShaderStageBits::Compute, vk::ShaderStageFlagBits::eCompute)
        FCIMPL_ITEM(RHIShaderStageBits::Geometry, vk::ShaderStageFlagBits::eGeometry)
        FCIMPL_ITEM(RHIShaderStageBits::Domain, vk::ShaderStageFlagBits::eTessellationControl)
        FCIMPL_ITEM(RHIShaderStageBits::Hull, vk::ShaderStageFlagBits::eTessellationEvaluation)
    FCIMPL_END(vk::ShaderStageFlagBits)

    FCIMPL_BEGIN(RHIBufferUsageFlags, vk::BufferUsageFlagBits)
        FCIMPL_ITEM(RHIBufferUsageBits::CopySrc, vk::BufferUsageFlagBits::eTransferSrc)
        FCIMPL_ITEM(RHIBufferUsageBits::CopyDst, vk::BufferUsageFlagBits::eTransferDst)
        FCIMPL_ITEM(RHIBufferUsageBits::Index, vk::BufferUsageFlagBits::eIndexBuffer)
        FCIMPL_ITEM(RHIBufferUsageBits::Vertex, vk::BufferUsageFlagBits::eVertexBuffer)
        FCIMPL_ITEM(RHIBufferUsageBits::Uniform, vk::BufferUsageFlagBits::eUniformBuffer)
        FCIMPL_ITEM(RHIBufferUsageBits::Storage, vk::BufferUsageFlagBits::eStorageBuffer)
        FCIMPL_ITEM(RHIBufferUsageBits::Indirect, vk::BufferUsageFlagBits::eIndirectBuffer)
    FCIMPL_END(vk::BufferUsageFlagBits)

    FCIMPL_BEGIN(RHITextureUsageFlags, vk::ImageUsageFlagBits)
        FCIMPL_ITEM(RHITextureUsageBits::CopySrc, vk::ImageUsageFlagBits::eTransferSrc)
        FCIMPL_ITEM(RHITextureUsageBits::CopyDst, vk::ImageUsageFlagBits::eTransferDst)
        FCIMPL_ITEM(RHITextureUsageBits::TextureBinding, vk::ImageUsageFlagBits::eSampled)
        FCIMPL_ITEM(RHITextureUsageBits::StorageBinding, vk::ImageUsageFlagBits::eStorage)
        FCIMPL_ITEM(RHITextureUsageBits::RenderAttachment, vk::ImageUsageFlagBits::eColorAttachment)
        FCIMPL_ITEM(RHITextureUsageBits::DepthStencilAttachment, vk::ImageUsageFlagBits::eDepthStencilAttachment)
    FCIMPL_END(vk::ImageUsageFlagBits)

    // inline vk::Extent3D GetVkExtent3D(const Math::Vector3u& ext)
    // {
    //     return { ext.x, ext.y, ext.z };
    // }

    // inline vk::ShaderStageFlags GetVkShaderStageFlags(const RHIShaderStageFlags& src)
    // {
    //     vk::ShaderStageFlags dst = {};
    //     for (auto& pair : GetEnumMap<RHIShaderStageFlags, vk::ShaderStageFlagBits>())
    //     {
    //         if (src & pair.first)
    //         {
    //             dst |= pair.second;
    //         }
    //     }
    //     return dst;
    // }

    // inline vk::ImageUsageFlags GetVkImageUsageFlags(const RHITextureUsageFlags& src)
    // {
    //     vk::ImageUsageFlags dst = {};
    //     for (auto& pair : GetEnumMap<RHITextureUsageFlags, vk::ImageUsageFlagBits>())
    //     {
    //         if (src & pair.first)
    //         {
    //             dst |= pair.second;
    //         }
    //     }
    //     return dst;
    // }

    // inline vk::ImageAspectFlags GetVkAspectMask(const RHITextureType& type)
    // {
    //     vk::ImageAspectFlags result {};
    //     for (const auto& pair : GetEnumMap<RHITextureType, vk::ImageAspectFlags>())
    //     {
    //         if (type == pair.first)
    //         {
    //             result = pair.second;
    //         }
    //     }
    //     return result;
    // }

    // inline vk::BufferUsageFlags GetVkBufferUsageFlags(const RHIBufferUsageFlags& bufferUsage)
    // {
    //     vk::BufferUsageFlags res {};
    //     for (const auto& pair : GetEnumMap<RHIBufferUsageFlags, vk::BufferUsageFlagBits>())
    //     {
    //         if (bufferUsage & pair.first)
    //         {
    //             res |= pair.second;
    //         }
    //     }
    //     return res;
    // }
}