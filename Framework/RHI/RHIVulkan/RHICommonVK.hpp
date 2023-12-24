#pragma once

#include "Utilities/Math.hpp"
#include "RHI/RHICommon.hpp"

#include <unordered_map>
#include <cassert>
#include <vulkan/vulkan.hpp>

namespace Vultana
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

    VK_ENUM_MAP_BEGIN(RHIShaderStageBits, vk::ShaderStageFlagBits)
        VK_ENUM_MAP_ITEM(RHIShaderStageBits::Vertex, vk::ShaderStageFlagBits::eVertex)
        VK_ENUM_MAP_ITEM(RHIShaderStageBits::Pixel, vk::ShaderStageFlagBits::eFragment)
        VK_ENUM_MAP_ITEM(RHIShaderStageBits::Compute, vk::ShaderStageFlagBits::eCompute)
    VK_ENUM_MAP_END()

    inline vk::Extent3D FromRHI(const Vector3& ext)
    {
        return { (uint32_t)ext.x, (uint32_t)ext.y, (uint32_t)ext.z };
    }

    inline vk::ShaderStageFlags FromRHI(const RHIShaderStageFlags& src)
    {
        vk::ShaderStageFlags dst = {};
        for (auto& pair : GetEnumMap<RHIShaderStageBits, vk::ShaderStageFlagBits>())
        {
            if (src & pair.first)
            {
                dst |= pair.second;
            }
        }
        return dst;
    }

    inline vk::ImageAspectFlags FromRHI(const RHITextureType type)
    {
        static std::unordered_map<RHITextureType, vk::ImageAspectFlags> map = {
            { RHITextureType::Color, vk::ImageAspectFlagBits::eColor },
            { RHITextureType::Depth, vk::ImageAspectFlagBits::eDepth },
            { RHITextureType::Stencil, vk::ImageAspectFlagBits::eStencil },
            { RHITextureType::DepthStencil, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil }
        };
        vk::ImageAspectFlags result = {};
        for (auto& pair : map)
        {
            if (type == pair.first)
            {
                result = pair.second;
                break;
            }
        }
        return result;
    }
}