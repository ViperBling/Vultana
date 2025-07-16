#include "RHIDescriptorVK.hpp"
#include "RHIBufferVK.hpp"
#include "RHITextureVK.hpp"
#include "RHIDeviceVK.hpp"

#include "Utilities/Log.hpp"

namespace RHI
{
    RHIShaderResourceViewVK::RHIShaderResourceViewVK(RHIDeviceVK *device, RHIResource *pResource, const RHIShaderResourceViewDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Name = name;
        m_Resource = pResource;
        m_Desc = desc;
    }

    RHIShaderResourceViewVK::~RHIShaderResourceViewVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(m_pDevice);
        device->Delete(m_ImageView);
        device->FreeResourceDescriptor(m_HeapIndex);
    }

    bool RHIShaderResourceViewVK::Create()
    {
        vk::Device deviceHandle = static_cast<RHIDeviceVK *>(m_pDevice)->GetDevice();
        auto dynamicLoader = static_cast<RHIDeviceVK *>(m_pDevice)->GetDynamicLoader();
        const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& descBufferProps = static_cast<RHIDeviceVK *>(m_pDevice)->GetDescriptorBufferProperties();

        size_t descSize = 0;
        vk::DescriptorGetInfoEXT descGetInfo {};
        vk::DescriptorAddressInfoEXT descBufferInfo {};
        vk::DescriptorImageInfo descImageInfo {};
        descImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        vk::ImageViewUsageCreateInfo imageViewUsageCI {};
        imageViewUsageCI.setUsage(vk::ImageUsageFlagBits::eSampled);

        vk::ImageViewCreateInfo imageViewCI {};
        if (m_Resource && m_Resource->IsTexture())
        {
            vk::ImageSubresourceRange subresourceRange {};
            subresourceRange.aspectMask = GetAspectFlags(m_Desc.Format);
            subresourceRange.baseMipLevel = m_Desc.Texture.MipSlice;
            subresourceRange.levelCount = m_Desc.Texture.MipLevels;
            subresourceRange.baseArrayLayer = m_Desc.Texture.ArraySlice;
            subresourceRange.layerCount = m_Desc.Texture.ArraySize;

            imageViewCI.setPNext(&imageViewUsageCI);
            imageViewCI.setImage((VkImage)m_Resource->GetNativeHandle());
            imageViewCI.setFormat(ToVulkanFormat(m_Desc.Format, true));
            imageViewCI.setSubresourceRange(subresourceRange);

            descGetInfo.type = vk::DescriptorType::eSampledImage;
            descGetInfo.data.pSampledImage = &descImageInfo;
            descSize = descBufferProps.sampledImageDescriptorSize;
        }

        switch (m_Desc.Type)
        {
        case ERHIShaderResourceViewType::Textue2D:
        {
            imageViewCI.setViewType(vk::ImageViewType::e2D);
            m_ImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(m_ImageView);
            break;
        }
        case ERHIShaderResourceViewType::Texture2DArray:
        {
            imageViewCI.setViewType(vk::ImageViewType::e2DArray);
            m_ImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(m_ImageView);
            break;
        }
        case ERHIShaderResourceViewType::Texture3D:
        {
            imageViewCI.setViewType(vk::ImageViewType::e3D);
            m_ImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(m_ImageView);
            break;
        }
        case ERHIShaderResourceViewType::TextureCube:
        {
            imageViewCI.setViewType(vk::ImageViewType::eCube);
            m_ImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(m_ImageView);
            break;
        }
        case ERHIShaderResourceViewType::TextureCubeArray:
        {
            imageViewCI.setViewType(vk::ImageViewType::eCubeArray);
            m_ImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(m_ImageView);
            break;
        }
        case ERHIShaderResourceViewType::StructuredBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(m_Resource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageStructuredBuffer);
            assert(m_Desc.Format == ERHIFormat::Unknown);
            assert(m_Desc.Buffer.Offset % bufferDesc.Stride == 0);
            assert(m_Desc.Buffer.Size % bufferDesc.Stride == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(m_Resource)->GetGPUAddress() + m_Desc.Buffer.Offset;
            descBufferInfo.range = m_Desc.Buffer.Size;

            descGetInfo.setType(vk::DescriptorType::eStorageBuffer);
            descGetInfo.data.pStorageBuffer = &descBufferInfo;
            descSize = descBufferProps.robustStorageBufferDescriptorSize;
            break;
        }
        case ERHIShaderResourceViewType::TypedBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(m_Resource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageTypedBuffer);
            assert(m_Desc.Buffer.Offset % bufferDesc.Stride == 0);
            assert(m_Desc.Buffer.Size % bufferDesc.Stride == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(m_Resource)->GetGPUAddress() + m_Desc.Buffer.Offset;
            descBufferInfo.range = m_Desc.Buffer.Size;
            descBufferInfo.format = ToVulkanFormat(m_Desc.Format);

            descGetInfo.setType(vk::DescriptorType::eUniformTexelBuffer);
            descGetInfo.data.pStorageBuffer = &descBufferInfo;
            descSize = descBufferProps.robustUniformTexelBufferDescriptorSize;
            break;
        }
        case ERHIShaderResourceViewType::RawBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(m_Resource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageRawBuffer);
            assert(bufferDesc.Stride % 4 == 0);
            assert(m_Desc.Buffer.Offset % 4 == 0);
            assert(m_Desc.Buffer.Size % 4 == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(m_Resource)->GetGPUAddress() + m_Desc.Buffer.Offset;
            descBufferInfo.range = m_Desc.Buffer.Size;

            descGetInfo.setType(vk::DescriptorType::eStorageBuffer);
            descGetInfo.data.pStorageBuffer = &descBufferInfo;
            descSize = descBufferProps.robustStorageBufferDescriptorSize;
            break;
        }
        default:
            break;
        }

        void* pDescriptor = nullptr;
        m_HeapIndex = static_cast<RHIDeviceVK *>(m_pDevice)->AllocateResourceDescriptor(&pDescriptor);

        deviceHandle.getDescriptorEXT(descGetInfo, descSize, pDescriptor, dynamicLoader);
        return true;
    }

    RHIUnorderedAccessViewVK::RHIUnorderedAccessViewVK(RHIDeviceVK *device, RHIResource *pResource, const RHIUnorderedAccessViewDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Name = name;
        m_Resource = pResource;
        m_Desc = desc;
    }

    RHIUnorderedAccessViewVK::~RHIUnorderedAccessViewVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(m_pDevice);
        device->Delete(m_ImageView);
        device->FreeResourceDescriptor(m_HeapIndex);
    }

    bool RHIUnorderedAccessViewVK::Create()
    {
        vk::Device deviceHandle = static_cast<RHIDeviceVK *>(m_pDevice)->GetDevice();
        auto dynamicLoader = static_cast<RHIDeviceVK *>(m_pDevice)->GetDynamicLoader();
        const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& descBufferProps = static_cast<RHIDeviceVK *>(m_pDevice)->GetDescriptorBufferProperties();

        size_t descSize = 0;
        vk::DescriptorGetInfoEXT descGetInfo {};
        vk::DescriptorAddressInfoEXT descBufferInfo {};
        vk::DescriptorImageInfo descImageInfo {};
        descImageInfo.imageLayout = vk::ImageLayout::eGeneral;

        vk::ImageViewUsageCreateInfo imageViewUsageCI {};
        imageViewUsageCI.setUsage(vk::ImageUsageFlagBits::eStorage);

        vk::ImageViewCreateInfo imageViewCI {};
        if (m_Resource && m_Resource->IsTexture())
        {
            const RHITextureDesc& textureDesc = static_cast<RHITexture*>(m_Resource)->GetDesc();
            assert(textureDesc.Usage & RHITextureUsageUnorderedAccess);

            vk::ImageSubresourceRange subresourceRange {};
            subresourceRange.aspectMask = GetAspectFlags(m_Desc.Format);
            subresourceRange.baseMipLevel = m_Desc.Texture.MipSlice;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = m_Desc.Texture.ArraySlice;
            subresourceRange.layerCount = m_Desc.Texture.ArraySize;

            imageViewCI.setPNext(&imageViewUsageCI);
            imageViewCI.setImage((VkImage)m_Resource->GetNativeHandle());
            imageViewCI.setFormat(ToVulkanFormat(m_Desc.Format));
            imageViewCI.setSubresourceRange(subresourceRange);

            descGetInfo.type = vk::DescriptorType::eStorageImage;
            descGetInfo.data.pStorageImage = &descImageInfo;
            descSize = descBufferProps.storageImageDescriptorSize;
        }

        switch (m_Desc.Type)
        {
        case ERHIUnorderedAccessViewType::Texture2D:
        {
            imageViewCI.setViewType(vk::ImageViewType::e2D);
            m_ImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(m_ImageView);
            break;
        }
        case ERHIUnorderedAccessViewType::Texture2DArray:
        {
            imageViewCI.setViewType(vk::ImageViewType::e2DArray);
            m_ImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(m_ImageView);
            break;
        }
        case ERHIUnorderedAccessViewType::Texture3D:
        {
            imageViewCI.setViewType(vk::ImageViewType::e3D);
            m_ImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(m_ImageView);
            break;
        }
        case ERHIUnorderedAccessViewType::StructuredBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(m_Resource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageStructuredBuffer);
            assert(bufferDesc.Usage & RHIBufferUsageUnorderedAccess);
            assert(m_Desc.Format == ERHIFormat::Unknown);
            assert(m_Desc.Buffer.Offset % bufferDesc.Stride == 0);
            assert(m_Desc.Buffer.Size % bufferDesc.Stride == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(m_Resource)->GetGPUAddress() + m_Desc.Buffer.Offset;
            descBufferInfo.range = m_Desc.Buffer.Size;

            descGetInfo.setType(vk::DescriptorType::eStorageBuffer);
            descGetInfo.data.pStorageBuffer = &descBufferInfo;
            descSize = descBufferProps.robustStorageBufferDescriptorSize;
            break;
        }
        case ERHIUnorderedAccessViewType::TypedBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(m_Resource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageTypedBuffer);
            assert(bufferDesc.Usage & RHIBufferUsageUnorderedAccess);
            assert(m_Desc.Buffer.Offset % bufferDesc.Stride == 0);
            assert(m_Desc.Buffer.Size % bufferDesc.Stride == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(m_Resource)->GetGPUAddress() + m_Desc.Buffer.Offset;
            descBufferInfo.range = m_Desc.Buffer.Size;
            descBufferInfo.format = ToVulkanFormat(m_Desc.Format);

            descGetInfo.setType(vk::DescriptorType::eStorageTexelBuffer);
            descGetInfo.data.pUniformTexelBuffer = &descBufferInfo;
            descSize = descBufferProps.robustUniformTexelBufferDescriptorSize;
            break;
        }
        case ERHIUnorderedAccessViewType::RawBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(m_Resource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageRawBuffer);
            assert(bufferDesc.Usage & RHIBufferUsageUnorderedAccess);
            assert(bufferDesc.Stride % 4 == 0);
            assert(m_Desc.Buffer.Offset % 4 == 0);
            assert(m_Desc.Buffer.Size % 4 == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(m_Resource)->GetGPUAddress() + m_Desc.Buffer.Offset;
            descBufferInfo.range = m_Desc.Buffer.Size;

            descGetInfo.setType(vk::DescriptorType::eStorageBuffer);
            descGetInfo.data.pStorageBuffer = &descBufferInfo;
            descSize = descBufferProps.robustStorageBufferDescriptorSize;
            break;
        }
        default:
            break;
        }

        void* pDescriptor = nullptr;
        m_HeapIndex = static_cast<RHIDeviceVK *>(m_pDevice)->AllocateResourceDescriptor(&pDescriptor);

        deviceHandle.getDescriptorEXT(descGetInfo, descSize, pDescriptor, dynamicLoader);
        return true;
    }

    RHIConstantBufferViewVK::RHIConstantBufferViewVK(RHIDeviceVK *device, RHIBuffer *buffer, const RHIConstantBufferViewDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Name = name;
        m_Buffer = buffer;
        m_Desc = desc;
    }

    RHIConstantBufferViewVK::~RHIConstantBufferViewVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(m_pDevice);
        device->FreeResourceDescriptor(m_HeapIndex);
    }

    bool RHIConstantBufferViewVK::Create()
    {
        vk::DescriptorAddressInfoEXT descBufferInfo {};
        descBufferInfo.address = m_Buffer->GetGPUAddress() + m_Desc.Offset;
        descBufferInfo.range = m_Desc.Size;

        vk::DescriptorGetInfoEXT descGetInfo {};
        descGetInfo.setType(vk::DescriptorType::eUniformBuffer);
        descGetInfo.setData(&descBufferInfo);

        void* pDescriptor = nullptr;
        m_HeapIndex = static_cast<RHIDeviceVK *>(m_pDevice)->AllocateResourceDescriptor(&pDescriptor);

        vk::Device deviceHandle = static_cast<RHIDeviceVK *>(m_pDevice)->GetDevice();
        auto dynamicLoader = static_cast<RHIDeviceVK *>(m_pDevice)->GetDynamicLoader();
        size_t size = static_cast<RHIDeviceVK *>(m_pDevice)->GetDescriptorBufferProperties().robustUniformBufferDescriptorSize;

        deviceHandle.getDescriptorEXT(descGetInfo, size, pDescriptor, dynamicLoader);

        return true;
    }

    RHISamplerVK::RHISamplerVK(RHIDeviceVK *device, const RHISamplerDesc &desc, const eastl::string &name)
    {
        m_pDevice = device;
        m_Name = name;
        m_Desc = desc;
    }

    RHISamplerVK::~RHISamplerVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(m_pDevice);
        device->Delete(m_Sampler);
        device->FreeSamplerDescriptor(m_HeapIndex);
    }

    bool RHISamplerVK::Create()
    {
        vk::SamplerReductionModeCreateInfo samplerReductionModeCI {};
        samplerReductionModeCI.setReductionMode(ToVKSamplerReductionMode(m_Desc.ReductionMode));

        vk::SamplerCreateInfo samplerCI {};
        samplerCI.setPNext(&samplerReductionModeCI);
        samplerCI.setMagFilter(ToVKFilter(m_Desc.MagFilter));
        samplerCI.setMinFilter(ToVKFilter(m_Desc.MinFilter));
        samplerCI.setMipmapMode(ToVKSamplerMipmapMode(m_Desc.MipFilter));
        samplerCI.setAddressModeU(ToVKSamplerAddressMode(m_Desc.AddressU));
        samplerCI.setAddressModeV(ToVKSamplerAddressMode(m_Desc.AddressV));
        samplerCI.setAddressModeW(ToVKSamplerAddressMode(m_Desc.AddressW));
        samplerCI.setMipLodBias(m_Desc.MipLODBias);
        samplerCI.setAnisotropyEnable(m_Desc.bEnableAnisotropy);
        samplerCI.setMaxAnisotropy(m_Desc.MaxAnisotropy);
        samplerCI.setCompareEnable(m_Desc.ReductionMode == ERHISamplerReductionMode::Compare);
        samplerCI.setMinLod(m_Desc.MinLOD);
        samplerCI.setMaxLod(m_Desc.MaxLOD);

        if (m_Desc.BorderColor[0] == 0.0f && m_Desc.BorderColor[1] == 0.0f && m_Desc.BorderColor[2] == 0.0f && m_Desc.BorderColor[3] == 0.0f)
        {
            samplerCI.setBorderColor(vk::BorderColor::eFloatTransparentBlack);
        }
        else if (m_Desc.BorderColor[0] == 0.0f && m_Desc.BorderColor[1] == 0.0f && m_Desc.BorderColor[2] == 0.0f && m_Desc.BorderColor[3] == 1.0f)
        {
            samplerCI.setBorderColor(vk::BorderColor::eFloatOpaqueBlack);
        }
        else if (m_Desc.BorderColor[0] == 1.0f && m_Desc.BorderColor[1] == 1.0f && m_Desc.BorderColor[2] == 1.0f && m_Desc.BorderColor[3] == 1.0f)
        {
            samplerCI.setBorderColor(vk::BorderColor::eFloatOpaqueWhite);
        }
        else
        {
            assert(false);
        }

        vk::Device deviceHandle = static_cast<RHIDeviceVK *>(m_pDevice)->GetDevice();
        auto dynamicLoader = static_cast<RHIDeviceVK *>(m_pDevice)->GetDynamicLoader();
        m_Sampler = deviceHandle.createSampler(samplerCI);
        if (!m_Sampler)
        {
            VTNA_LOG_ERROR("[RHISamplerVK] Failed to create {}", m_Name);
            return false;
        }

        void* pDescriptor = nullptr;
        m_HeapIndex = static_cast<RHIDeviceVK *>(m_pDevice)->AllocateSamplerDescriptor(&pDescriptor);

        vk::DescriptorGetInfoEXT descGetInfo {};
        descGetInfo.setType(vk::DescriptorType::eSampler);
        descGetInfo.data.pSampler = &m_Sampler;

        size_t size = static_cast<RHIDeviceVK *>(m_pDevice)->GetDescriptorBufferProperties().samplerDescriptorSize;
        deviceHandle.getDescriptorEXT(descGetInfo, size, pDescriptor, dynamicLoader);

        return true;
    }
}