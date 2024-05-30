#pragma once

#include "RHI/RHICommon.hpp"

#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <cassert>

namespace RHI::Vulkan
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

    VK_ENUM_MAP_BEGIN(ERHIFormat, vk::Format)
        VK_ENUM_MAP_ITEM(ERHIFormat::Unknown,       vk::Format::eUndefined)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA32F,       vk::Format::eR32G32B32A32Sfloat)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA32UI,      vk::Format::eR32G32B32A32Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA32SI,      vk::Format::eR32G32B32A32Sint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA16F,       vk::Format::eR16G16B16A16Sfloat)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA16UI,      vk::Format::eR16G16B16A16Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA16SI,      vk::Format::eR16G16B16A16Sint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA16UNORM,   vk::Format::eR16G16B16A16Unorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA16SNORM,   vk::Format::eR16G16B16A16Snorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA8UI,       vk::Format::eR8G8B8A8Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA8SI,       vk::Format::eR8G8B8A8Sint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA8UNORM,    vk::Format::eR8G8B8A8Unorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA8SNORM,    vk::Format::eR8G8B8A8Snorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGBA8SRGB,     vk::Format::eR8G8B8A8Srgb)
        VK_ENUM_MAP_ITEM(ERHIFormat::BGRA8UNORM,    vk::Format::eB8G8R8A8Unorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::BGRA8SRGB,     vk::Format::eB8G8R8A8Srgb)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGB10A2UI,     vk::Format::eA2B10G10R10UintPack32)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGB10A2UNORM,  vk::Format::eA2B10G10R10UnormPack32)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGB32F,        vk::Format::eR32G32B32Sfloat)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGB32UI,       vk::Format::eR32G32B32Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGB32SI,       vk::Format::eR32G32B32Sint)
        VK_ENUM_MAP_ITEM(ERHIFormat::R11G11B10F,    vk::Format::eB10G11R11UfloatPack32)
        VK_ENUM_MAP_ITEM(ERHIFormat::RGB9E5,        vk::Format::eE5B9G9R9UfloatPack32)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG32F,         vk::Format::eR32G32Sfloat)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG32UI,        vk::Format::eR32G32Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG32SI,        vk::Format::eR32G32Sint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG16F,         vk::Format::eR16G16Sfloat)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG16UI,        vk::Format::eR16G16Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG16SI,        vk::Format::eR16G16Sint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG16UNORM,     vk::Format::eR16G16Unorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG16SNORM,     vk::Format::eR16G16Snorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG8UI,         vk::Format::eR8G8Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG8SI,         vk::Format::eR8G8Sint)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG8UNORM,      vk::Format::eR8G8Unorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::RG8SNORM,      vk::Format::eR8G8Snorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::R32F,          vk::Format::eR32Sfloat)
        VK_ENUM_MAP_ITEM(ERHIFormat::R32UI,         vk::Format::eR32Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::R32SI,         vk::Format::eR32Sint)
        VK_ENUM_MAP_ITEM(ERHIFormat::R16F,          vk::Format::eR16Sfloat)
        VK_ENUM_MAP_ITEM(ERHIFormat::R16UI,         vk::Format::eR16Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::R16SI,         vk::Format::eR16Sint)
        VK_ENUM_MAP_ITEM(ERHIFormat::R16UNORM,      vk::Format::eR16Unorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::R16SNORM,      vk::Format::eR16Snorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::R8UI,          vk::Format::eR8Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::R8SI,          vk::Format::eR8Sint)
        VK_ENUM_MAP_ITEM(ERHIFormat::R8UNORM,       vk::Format::eR8Unorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::R8SNORM,       vk::Format::eR8Snorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::D32F,          vk::Format::eD32Sfloat)
        VK_ENUM_MAP_ITEM(ERHIFormat::D32FS8,        vk::Format::eD32SfloatS8Uint)
        VK_ENUM_MAP_ITEM(ERHIFormat::D16,           vk::Format::eD16Unorm)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC1UNORM,      vk::Format::eBc1RgbaUnormBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC1SRGB,       vk::Format::eBc1RgbaSrgbBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC2UNORM,      vk::Format::eBc2UnormBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC2SRGB,       vk::Format::eBc2SrgbBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC3UNORM,      vk::Format::eBc3UnormBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC3SRGB,       vk::Format::eBc3SrgbBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC4UNORM,      vk::Format::eBc4UnormBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC4SNORM,      vk::Format::eBc4SnormBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC5UNORM,      vk::Format::eBc5UnormBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC5SNORM,      vk::Format::eBc5SnormBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC6U16F,       vk::Format::eBc6HUfloatBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC6S16F,       vk::Format::eBc6HSfloatBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC7UNORM,      vk::Format::eBc7UnormBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::BC7SRGB,       vk::Format::eBc7SrgbBlock)
        VK_ENUM_MAP_ITEM(ERHIFormat::Count,         vk::Format::eUndefined)
    VK_ENUM_MAP_END()
}