#pragma once

#include "Utilities/Math.hpp"
#include "RHI/RHICommon.hpp"

#include <unordered_map>
#include <cassert>
#include <vulkan/vulkan.hpp>

namespace RHI
{
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

    #define VK_ENUM_MAP_BEGIN(A, B) template <> std::unordered_map<A, B>& GetEnumMap<A, B>() { static std::unordered_map<A, B> map = {
    #define VK_ENUM_MAP_ITEM(A, B) { A, B },
    #define VK_ENUM_MAP_END() }; return map; }

    VK_ENUM_MAP_BEGIN(vk::PhysicalDeviceType, RHIDeviceType)
        VK_ENUM_MAP_ITEM(vk::PhysicalDeviceType::eOther, RHIDeviceType::Software)
        VK_ENUM_MAP_ITEM(vk::PhysicalDeviceType::eIntegratedGpu, RHIDeviceType::Software)
        VK_ENUM_MAP_ITEM(vk::PhysicalDeviceType::eDiscreteGpu, RHIDeviceType::Hardware)
        VK_ENUM_MAP_ITEM(vk::PhysicalDeviceType::eVirtualGpu, RHIDeviceType::Software)
        VK_ENUM_MAP_ITEM(vk::PhysicalDeviceType::eCpu, RHIDeviceType::Software)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIFormat, vk::Format)
        VK_ENUM_MAP_ITEM(RHIFormat::R8_UNORM, vk::Format::eR8Unorm)
        VK_ENUM_MAP_ITEM(RHIFormat::R8_SNORM, vk::Format::eR8Snorm)
        VK_ENUM_MAP_ITEM(RHIFormat::R8_UINT, vk::Format::eR8Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::R8_SINT, vk::Format::eR8Sint)
        VK_ENUM_MAP_ITEM(RHIFormat::R16_UINT, vk::Format::eR16Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::R16_SINT, vk::Format::eR16Sint)
        VK_ENUM_MAP_ITEM(RHIFormat::R16_FLOAT, vk::Format::eR16Sfloat)
        VK_ENUM_MAP_ITEM(RHIFormat::RG8_UNORM, vk::Format::eR8G8Unorm)
        VK_ENUM_MAP_ITEM(RHIFormat::RG8_SNORM, vk::Format::eR8G8Snorm)
        VK_ENUM_MAP_ITEM(RHIFormat::RG8_UINT, vk::Format::eR8G8Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::RG8_SINT, vk::Format::eR8G8Sint)
        VK_ENUM_MAP_ITEM(RHIFormat::R32_UINT, vk::Format::eR32Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::R32_SINT, vk::Format::eR32Sint)
        VK_ENUM_MAP_ITEM(RHIFormat::R32_FLOAT, vk::Format::eR32Sfloat)
        VK_ENUM_MAP_ITEM(RHIFormat::RG16_UINT, vk::Format::eR16G16Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::RG16_SINT, vk::Format::eR16G16Sint)
        VK_ENUM_MAP_ITEM(RHIFormat::RG16_FLOAT, vk::Format::eR16G16Sfloat)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA8_UNORM, vk::Format::eR8G8B8A8Unorm)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA8_UNORM_SRGB, vk::Format::eR8G8B8A8Srgb)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA8_SNORM, vk::Format::eR8G8B8A8Snorm)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA8_UINT, vk::Format::eR8G8B8A8Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA8_SINT, vk::Format::eR8G8B8A8Sint)
        VK_ENUM_MAP_ITEM(RHIFormat::BGRA8_UNORM, vk::Format::eB8G8R8A8Unorm)
        VK_ENUM_MAP_ITEM(RHIFormat::BGRA8_UNORM_SRGB, vk::Format::eB8G8R8A8Srgb)
        VK_ENUM_MAP_ITEM(RHIFormat::RGB9E5_FLOAT, vk::Format::eE5B9G9R9UfloatPack32)
        VK_ENUM_MAP_ITEM(RHIFormat::RGB10A2_UNORM, vk::Format::eA2B10G10R10UnormPack32)
        VK_ENUM_MAP_ITEM(RHIFormat::RG11B10_FLOAT, vk::Format::eB10G11R11UfloatPack32)
        VK_ENUM_MAP_ITEM(RHIFormat::RG32_UINT, vk::Format::eR32G32Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::RG32_SINT, vk::Format::eR32G32Sint)
        VK_ENUM_MAP_ITEM(RHIFormat::RG32_FLOAT, vk::Format::eR32G32Sfloat)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA16_UINT, vk::Format::eR16G16B16A16Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA16_SINT, vk::Format::eR16G16B16A16Sint)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA16_FLOAT, vk::Format::eR16G16B16A16Sfloat)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA32_UINT, vk::Format::eR32G32B32A32Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA32_SINT, vk::Format::eR32G32B32A32Sint)
        VK_ENUM_MAP_ITEM(RHIFormat::RGBA32_FLOAT, vk::Format::eR32G32B32A32Sfloat)
        VK_ENUM_MAP_ITEM(RHIFormat::D16_UNORM, vk::Format::eD16Unorm)
        VK_ENUM_MAP_ITEM(RHIFormat::D24_UNORM_S8_UINT, vk::Format::eD24UnormS8Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::D32_FLOAT, vk::Format::eD32Sfloat)
        VK_ENUM_MAP_ITEM(RHIFormat::D32_FLOAT_S8_UINT, vk::Format::eD32SfloatS8Uint)
        VK_ENUM_MAP_ITEM(RHIFormat::Count, vk::Format::eUndefined)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHICommandQueueType, vk::QueueFlagBits)
        VK_ENUM_MAP_ITEM(RHICommandQueueType::Graphics, vk::QueueFlagBits::eGraphics)
        VK_ENUM_MAP_ITEM(RHICommandQueueType::Compute, vk::QueueFlagBits::eCompute)
        VK_ENUM_MAP_ITEM(RHICommandQueueType::Transfer, vk::QueueFlagBits::eTransfer)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHITextureDimension, vk::ImageType)
        VK_ENUM_MAP_ITEM(RHITextureDimension::Texture1D, vk::ImageType::e1D)
        VK_ENUM_MAP_ITEM(RHITextureDimension::Texture2D, vk::ImageType::e2D)
        VK_ENUM_MAP_ITEM(RHITextureDimension::Texture3D, vk::ImageType::e3D)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHITextureViewDimension, vk::ImageViewType)
        VK_ENUM_MAP_ITEM(RHITextureViewDimension::TextureView1D, vk::ImageViewType::e1D)
        VK_ENUM_MAP_ITEM(RHITextureViewDimension::TextureView2D, vk::ImageViewType::e2D)
        VK_ENUM_MAP_ITEM(RHITextureViewDimension::TextureView2DArray, vk::ImageViewType::e2DArray)
        VK_ENUM_MAP_ITEM(RHITextureViewDimension::TextureViewCube, vk::ImageViewType::eCube)
        VK_ENUM_MAP_ITEM(RHITextureViewDimension::TextureViewCubeArray, vk::ImageViewType::eCubeArray)
        VK_ENUM_MAP_ITEM(RHITextureViewDimension::TextureView3D, vk::ImageViewType::e3D)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHITextureType, vk::ImageAspectFlags)
        VK_ENUM_MAP_ITEM(RHITextureType::Color, vk::ImageAspectFlagBits::eColor)
        VK_ENUM_MAP_ITEM(RHITextureType::Depth, vk::ImageAspectFlagBits::eDepth)
        VK_ENUM_MAP_ITEM(RHITextureType::Stencil, vk::ImageAspectFlagBits::eStencil)
        VK_ENUM_MAP_ITEM(RHITextureType::DepthStencil, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIShaderStageFlags, vk::ShaderStageFlagBits)
        VK_ENUM_MAP_ITEM(RHIShaderStageBits::Vertex, vk::ShaderStageFlagBits::eVertex)
        VK_ENUM_MAP_ITEM(RHIShaderStageBits::Pixel, vk::ShaderStageFlagBits::eFragment)
        VK_ENUM_MAP_ITEM(RHIShaderStageBits::Compute, vk::ShaderStageFlagBits::eCompute)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIBufferUsageFlags, vk::BufferUsageFlagBits)
        VK_ENUM_MAP_ITEM(RHIBufferUsageBits::CopySrc, vk::BufferUsageFlagBits::eTransferSrc)
        VK_ENUM_MAP_ITEM(RHIBufferUsageBits::CopyDst, vk::BufferUsageFlagBits::eTransferDst)
        VK_ENUM_MAP_ITEM(RHIBufferUsageBits::Index, vk::BufferUsageFlagBits::eIndexBuffer)
        VK_ENUM_MAP_ITEM(RHIBufferUsageBits::Vertex, vk::BufferUsageFlagBits::eVertexBuffer)
        VK_ENUM_MAP_ITEM(RHIBufferUsageBits::Uniform, vk::BufferUsageFlagBits::eUniformBuffer)
        VK_ENUM_MAP_ITEM(RHIBufferUsageBits::Storage, vk::BufferUsageFlagBits::eStorageBuffer)
        VK_ENUM_MAP_ITEM(RHIBufferUsageBits::Indirect, vk::BufferUsageFlagBits::eIndirectBuffer)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIPrimitiveTopologyType, vk::PrimitiveTopology)
        VK_ENUM_MAP_ITEM(RHIPrimitiveTopologyType::Point, vk::PrimitiveTopology::ePointList)
        VK_ENUM_MAP_ITEM(RHIPrimitiveTopologyType::Line, vk::PrimitiveTopology::eLineList)
        VK_ENUM_MAP_ITEM(RHIPrimitiveTopologyType::Triangle, vk::PrimitiveTopology::eTriangleList)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHICullMode, vk::CullModeFlagBits)
        VK_ENUM_MAP_ITEM(RHICullMode::None, vk::CullModeFlagBits::eNone)
        VK_ENUM_MAP_ITEM(RHICullMode::Front, vk::CullModeFlagBits::eFront)
        VK_ENUM_MAP_ITEM(RHICullMode::Back, vk::CullModeFlagBits::eBack)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIBlendOp, vk::BlendOp)
        VK_ENUM_MAP_ITEM(RHIBlendOp::Add, vk::BlendOp::eAdd)
        VK_ENUM_MAP_ITEM(RHIBlendOp::Subtract, vk::BlendOp::eSubtract)
        VK_ENUM_MAP_ITEM(RHIBlendOp::ReverseSubtract, vk::BlendOp::eReverseSubtract)
        VK_ENUM_MAP_ITEM(RHIBlendOp::Min, vk::BlendOp::eMin)
        VK_ENUM_MAP_ITEM(RHIBlendOp::Max, vk::BlendOp::eMax)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIBlendFactor, vk::BlendFactor)
        VK_ENUM_MAP_ITEM(RHIBlendFactor::Zero, vk::BlendFactor::eZero)
        VK_ENUM_MAP_ITEM(RHIBlendFactor::One, vk::BlendFactor::eOne)
        VK_ENUM_MAP_ITEM(RHIBlendFactor::Src, vk::BlendFactor::eSrcColor)
        VK_ENUM_MAP_ITEM(RHIBlendFactor::OneMinusSrc, vk::BlendFactor::eOneMinusSrcColor)
        VK_ENUM_MAP_ITEM(RHIBlendFactor::SrcAlpha, vk::BlendFactor::eSrcAlpha)
        VK_ENUM_MAP_ITEM(RHIBlendFactor::OneMinusSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha)
        VK_ENUM_MAP_ITEM(RHIBlendFactor::Dst, vk::BlendFactor::eDstColor)
        VK_ENUM_MAP_ITEM(RHIBlendFactor::OneMinusDst, vk::BlendFactor::eOneMinusDstColor)
        VK_ENUM_MAP_ITEM(RHIBlendFactor::DstAlpha, vk::BlendFactor::eDstAlpha)
        VK_ENUM_MAP_ITEM(RHIBlendFactor::OneMinusDstAlpha, vk::BlendFactor::eOneMinusDstAlpha)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIVertexFormat, vk::Format)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UINT_8X2, vk::Format::eR8G8Uint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UINT_8X4, vk::Format::eR8G8B8A8Uint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SINT_8X2, vk::Format::eR8G8Sint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SINT_8X4, vk::Format::eR8G8B8A8Sint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UNORM_8X2, vk::Format::eR8G8Unorm)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UNORM_8X4, vk::Format::eR8G8B8A8Unorm)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SNORM_8X2, vk::Format::eR8G8Snorm)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SNORM_8X4, vk::Format::eR8G8B8A8Snorm)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UINT_16X2, vk::Format::eR16G16Uint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UINT_16X4, vk::Format::eR16G16B16A16Uint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SINT_16X2, vk::Format::eR16G16Sint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SINT_16X4, vk::Format::eR16G16B16A16Sint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UNORM_16X2, vk::Format::eR16G16Unorm)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UNORM_16X4, vk::Format::eR16G16B16A16Unorm)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SNORM_16X2, vk::Format::eR16G16Snorm)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SNORM_16X4, vk::Format::eR16G16B16A16Snorm)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::FLOAT_16X2, vk::Format::eR16G16Sfloat)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::FLOAT_16X4, vk::Format::eR16G16B16A16Sfloat)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::FLOAT_32X1, vk::Format::eR32Sfloat)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::FLOAT_32X2, vk::Format::eR32G32Sfloat)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::FLOAT_32X3, vk::Format::eR32G32B32Sfloat)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::FLOAT_32X4, vk::Format::eR32G32B32A32Sfloat)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UINT_32X1, vk::Format::eR32Uint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UINT_32X2, vk::Format::eR32G32Uint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UINT_32X3, vk::Format::eR32G32B32Uint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::UINT_32X4, vk::Format::eR32G32B32A32Uint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SINT_32X1, vk::Format::eR32Sint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SINT_32X2, vk::Format::eR32G32Sint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SINT_32X3, vk::Format::eR32G32B32Sint)
        VK_ENUM_MAP_ITEM(RHIVertexFormat::SINT_32X4, vk::Format::eR32G32B32A32Sint)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIIndexFormat, vk::IndexType)
        VK_ENUM_MAP_ITEM(RHIIndexFormat::UINT_16, vk::IndexType::eUint16)
        VK_ENUM_MAP_ITEM(RHIIndexFormat::UINT_32, vk::IndexType::eUint32)
        VK_ENUM_MAP_ITEM(RHIIndexFormat::Count, vk::IndexType::eNoneKHR)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIBindingType, vk::DescriptorType)
        VK_ENUM_MAP_ITEM(RHIBindingType::UniformBuffer, vk::DescriptorType::eUniformBuffer)
        VK_ENUM_MAP_ITEM(RHIBindingType::StorageBuffer, vk::DescriptorType::eStorageBuffer)
        VK_ENUM_MAP_ITEM(RHIBindingType::Sampler, vk::DescriptorType::eSampler)
        VK_ENUM_MAP_ITEM(RHIBindingType::Texture, vk::DescriptorType::eSampledImage)
        VK_ENUM_MAP_ITEM(RHIBindingType::StorageTexture, vk::DescriptorType::eStorageImage)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHISamplerAddressMode, vk::SamplerAddressMode)
        VK_ENUM_MAP_ITEM(RHISamplerAddressMode::Clamp, vk::SamplerAddressMode::eClampToEdge)
        VK_ENUM_MAP_ITEM(RHISamplerAddressMode::Repeat, vk::SamplerAddressMode::eRepeat)
        VK_ENUM_MAP_ITEM(RHISamplerAddressMode::Mirror, vk::SamplerAddressMode::eMirroredRepeat)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHISamplerFilterMode, vk::Filter)
        VK_ENUM_MAP_ITEM(RHISamplerFilterMode::Nearest, vk::Filter::eNearest)
        VK_ENUM_MAP_ITEM(RHISamplerFilterMode::Linear, vk::Filter::eLinear)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHISamplerFilterMode, vk::SamplerMipmapMode)
        VK_ENUM_MAP_ITEM(RHISamplerFilterMode::Nearest, vk::SamplerMipmapMode::eNearest)
        VK_ENUM_MAP_ITEM(RHISamplerFilterMode::Linear, vk::SamplerMipmapMode::eLinear)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHICompareOp, vk::CompareOp)
        VK_ENUM_MAP_ITEM(RHICompareOp::Never, vk::CompareOp::eNever)
        VK_ENUM_MAP_ITEM(RHICompareOp::Less, vk::CompareOp::eLess)
        VK_ENUM_MAP_ITEM(RHICompareOp::Equal, vk::CompareOp::eEqual)
        VK_ENUM_MAP_ITEM(RHICompareOp::LessEqual, vk::CompareOp::eLessOrEqual)
        VK_ENUM_MAP_ITEM(RHICompareOp::Greater, vk::CompareOp::eGreater)
        VK_ENUM_MAP_ITEM(RHICompareOp::NotEqual, vk::CompareOp::eNotEqual)
        VK_ENUM_MAP_ITEM(RHICompareOp::GreaterEqual, vk::CompareOp::eGreaterOrEqual)
        VK_ENUM_MAP_ITEM(RHICompareOp::Always, vk::CompareOp::eAlways)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIStencilOp, vk::StencilOp)
        VK_ENUM_MAP_ITEM(RHIStencilOp::Keep, vk::StencilOp::eKeep)
        VK_ENUM_MAP_ITEM(RHIStencilOp::Zero, vk::StencilOp::eZero)
        VK_ENUM_MAP_ITEM(RHIStencilOp::Replace, vk::StencilOp::eReplace)
        VK_ENUM_MAP_ITEM(RHIStencilOp::IncrementClamp, vk::StencilOp::eIncrementAndClamp)
        VK_ENUM_MAP_ITEM(RHIStencilOp::DecrementClamp, vk::StencilOp::eDecrementAndClamp)
        VK_ENUM_MAP_ITEM(RHIStencilOp::IncrementWrap, vk::StencilOp::eIncrementAndWrap)
        VK_ENUM_MAP_ITEM(RHIStencilOp::DecrementWrap, vk::StencilOp::eDecrementAndWrap)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHILoadOp, vk::AttachmentLoadOp)
        VK_ENUM_MAP_ITEM(RHILoadOp::Load, vk::AttachmentLoadOp::eLoad)
        VK_ENUM_MAP_ITEM(RHILoadOp::Clear, vk::AttachmentLoadOp::eClear)
        VK_ENUM_MAP_ITEM(RHILoadOp::Count, vk::AttachmentLoadOp::eNoneEXT)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIStoreOp, vk::AttachmentStoreOp)
        VK_ENUM_MAP_ITEM(RHIStoreOp::Store, vk::AttachmentStoreOp::eStore)
        VK_ENUM_MAP_ITEM(RHIStoreOp::Discard, vk::AttachmentStoreOp::eDontCare)
        VK_ENUM_MAP_ITEM(RHIStoreOp::Count, vk::AttachmentStoreOp::eNoneEXT)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHITextureState, vk::ImageLayout)
        VK_ENUM_MAP_ITEM(RHITextureState::Undefined, vk::ImageLayout::eUndefined)
        VK_ENUM_MAP_ITEM(RHITextureState::RenderTarget, vk::ImageLayout::eColorAttachmentOptimal)
        VK_ENUM_MAP_ITEM(RHITextureState::Present, vk::ImageLayout::ePresentSrcKHR)
        VK_ENUM_MAP_ITEM(RHITextureState::Count, vk::ImageLayout::eGeneral)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHIPresentMode, vk::PresentModeKHR)
        VK_ENUM_MAP_ITEM(RHIPresentMode::Immediate, vk::PresentModeKHR::eImmediate)
        VK_ENUM_MAP_ITEM(RHIPresentMode::Vsync, vk::PresentModeKHR::eFifo)
        VK_ENUM_MAP_ITEM(RHIPresentMode::Count, vk::PresentModeKHR::eImmediate)
    VK_ENUM_MAP_END()

    VK_ENUM_MAP_BEGIN(RHITextureUsageFlags, vk::ImageUsageFlagBits)
        VK_ENUM_MAP_ITEM(RHITextureUsageBits::CopySrc, vk::ImageUsageFlagBits::eTransferSrc)
        VK_ENUM_MAP_ITEM(RHITextureUsageBits::CopyDst, vk::ImageUsageFlagBits::eTransferDst)
        VK_ENUM_MAP_ITEM(RHITextureUsageBits::TextureBinding, vk::ImageUsageFlagBits::eSampled)
        VK_ENUM_MAP_ITEM(RHITextureUsageBits::StorageBinding, vk::ImageUsageFlagBits::eStorage)
        VK_ENUM_MAP_ITEM(RHITextureUsageBits::RenderAttachment, vk::ImageUsageFlagBits::eColorAttachment)
        VK_ENUM_MAP_ITEM(RHITextureUsageBits::DepthStencilAttachment, vk::ImageUsageFlagBits::eDepthStencilAttachment)
    VK_ENUM_MAP_END()

    inline vk::Extent3D GetVkExtent3D(const Math::Vector3u& ext)
    {
        return { ext.x, ext.y, ext.z };
    }

    inline vk::ShaderStageFlags GetVkShaderStageFlags(const RHIShaderStageFlags& src)
    {
        vk::ShaderStageFlags dst = {};
        for (auto& pair : GetEnumMap<RHIShaderStageFlags, vk::ShaderStageFlagBits>())
        {
            if (src & pair.first)
            {
                dst |= pair.second;
            }
        }
        return dst;
    }

    inline vk::ImageUsageFlags GetVkImageUsageFlags(const RHITextureUsageFlags& src)
    {
        vk::ImageUsageFlags dst = {};
        for (auto& pair : GetEnumMap<RHITextureUsageFlags, vk::ImageUsageFlagBits>())
        {
            if (src & pair.first)
            {
                dst |= pair.second;
            }
        }
        return dst;
    }

    inline vk::ImageAspectFlags GetVkAspectMask(const RHITextureType& type)
    {
        vk::ImageAspectFlags result {};
        for (const auto& pair : GetEnumMap<RHITextureType, vk::ImageAspectFlags>())
        {
            if (type == pair.first)
            {
                result = pair.second;
            }
        }
        return result;
    }

    inline vk::BufferUsageFlags GetVkBufferUsageFlags(const RHIBufferUsageFlags& bufferUsage)
    {
        vk::BufferUsageFlags res {};
        for (const auto& pair : GetEnumMap<RHIBufferUsageFlags, vk::BufferUsageFlagBits>())
        {
            if (bufferUsage & pair.first)
            {
                res |= pair.second;
            }
        }
        return res;
    }
}