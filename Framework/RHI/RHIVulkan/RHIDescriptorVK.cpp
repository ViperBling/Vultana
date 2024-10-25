#include "RHIDescriptorVK.hpp"
#include "RHIBufferVK.hpp"
#include "RHITextureVK.hpp"
#include "RHIDeviceVK.hpp"

#include "Utilities/Log.hpp"

namespace RHI
{
    RHIShaderResourceViewVK::RHIShaderResourceViewVK(RHIDeviceVK *device, RHIResource *pResource, const RHIShaderResourceViewDesc &desc, const eastl::string &name)
    {
        mpDevice = device;
        mName = name;
        mResource = pResource;
        mDesc = desc;
    }

    RHIShaderResourceViewVK::~RHIShaderResourceViewVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(mpDevice);
        device->Delete(mImageView);
        device->FreeResourceDescriptor(mHeapIndex);
    }

    bool RHIShaderResourceViewVK::Create()
    {
        vk::Device deviceHandle = static_cast<RHIDeviceVK *>(mpDevice)->GetDevice();
        auto dynamicLoader = static_cast<RHIDeviceVK *>(mpDevice)->GetDynamicLoader();
        const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& descBufferProps = static_cast<RHIDeviceVK *>(mpDevice)->GetDescriptorBufferProperties();

        size_t descSize = 0;
        vk::DescriptorGetInfoEXT descGetInfo {};
        vk::DescriptorAddressInfoEXT descBufferInfo {};
        vk::DescriptorImageInfo descImageInfo {};
        descImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        vk::ImageViewUsageCreateInfo imageViewUsageCI {};
        imageViewUsageCI.setUsage(vk::ImageUsageFlagBits::eSampled);

        vk::ImageViewCreateInfo imageViewCI {};
        if (mResource && mResource->IsTexture())
        {
            vk::ImageSubresourceRange subresourceRange {};
            subresourceRange.aspectMask = GetAspectFlags(mDesc.Format);
            subresourceRange.baseMipLevel = mDesc.Texture.MipSlice;
            subresourceRange.levelCount = mDesc.Texture.MipLevels;
            subresourceRange.baseArrayLayer = mDesc.Texture.ArraySlice;
            subresourceRange.layerCount = mDesc.Texture.ArraySize;

            imageViewCI.setPNext(&imageViewUsageCI);
            imageViewCI.setImage((VkImage)mResource->GetNativeHandle());
            imageViewCI.setFormat(ToVulkanFormat(mDesc.Format, true));
            imageViewCI.setSubresourceRange(subresourceRange);

            descGetInfo.type = vk::DescriptorType::eSampledImage;
            descGetInfo.data.pSampledImage = &descImageInfo;
            descSize = descBufferProps.sampledImageDescriptorSize;
        }

        switch (mDesc.Type)
        {
        case ERHIShaderResourceViewType::Textue2D:
        {
            imageViewCI.setViewType(vk::ImageViewType::e2D);
            mImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(mImageView);
            break;
        }
        case ERHIShaderResourceViewType::Texture2DArray:
        {
            imageViewCI.setViewType(vk::ImageViewType::e2DArray);
            mImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(mImageView);
            break;
        }
        case ERHIShaderResourceViewType::Texture3D:
        {
            imageViewCI.setViewType(vk::ImageViewType::e3D);
            mImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(mImageView);
            break;
        }
        case ERHIShaderResourceViewType::TextureCube:
        {
            imageViewCI.setViewType(vk::ImageViewType::eCube);
            mImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(mImageView);
            break;
        }
        case ERHIShaderResourceViewType::TextureCubeArray:
        {
            imageViewCI.setViewType(vk::ImageViewType::eCubeArray);
            mImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(mImageView);
            break;
        }
        case ERHIShaderResourceViewType::StructuredBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(mResource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageStructuredBuffer);
            assert(mDesc.Format == ERHIFormat::Unknown);
            assert(mDesc.Buffer.Offset % bufferDesc.Stride == 0);
            assert(mDesc.Buffer.Size % bufferDesc.Stride == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(mResource)->GetGPUAddress() + mDesc.Buffer.Offset;
            descBufferInfo.range = mDesc.Buffer.Size;

            descGetInfo.setType(vk::DescriptorType::eStorageBuffer);
            descGetInfo.data.pStorageBuffer = &descBufferInfo;
            descSize = descBufferProps.robustStorageBufferDescriptorSize;
            break;
        }
        case ERHIShaderResourceViewType::TypedBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(mResource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageTypedBuffer);
            assert(mDesc.Buffer.Offset % bufferDesc.Stride == 0);
            assert(mDesc.Buffer.Size % bufferDesc.Stride == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(mResource)->GetGPUAddress() + mDesc.Buffer.Offset;
            descBufferInfo.range = mDesc.Buffer.Size;
            descBufferInfo.format = ToVulkanFormat(mDesc.Format);

            descGetInfo.setType(vk::DescriptorType::eUniformTexelBuffer);
            descGetInfo.data.pStorageBuffer = &descBufferInfo;
            descSize = descBufferProps.robustUniformTexelBufferDescriptorSize;
            break;
        }
        case ERHIShaderResourceViewType::RawBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(mResource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageRawBuffer);
            assert(bufferDesc.Stride % 4 == 0);
            assert(mDesc.Buffer.Offset % 4 == 0);
            assert(mDesc.Buffer.Size % 4 == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(mResource)->GetGPUAddress() + mDesc.Buffer.Offset;
            descBufferInfo.range = mDesc.Buffer.Size;

            descGetInfo.setType(vk::DescriptorType::eStorageBuffer);
            descGetInfo.data.pStorageBuffer = &descBufferInfo;
            descSize = descBufferProps.robustStorageBufferDescriptorSize;
            break;
        }
        default:
            break;
        }

        void* pDescriptor = nullptr;
        mHeapIndex = static_cast<RHIDeviceVK *>(mpDevice)->AllocateResourceDescriptor(&pDescriptor);

        deviceHandle.getDescriptorEXT(descGetInfo, descSize, pDescriptor, dynamicLoader);
        return true;
    }

    RHIUnorderedAccessViewVK::RHIUnorderedAccessViewVK(RHIDeviceVK *device, RHIResource *pResource, const RHIUnorderedAccessViewDesc &desc, const eastl::string &name)
    {
        mpDevice = device;
        mName = name;
        mResource = pResource;
        mDesc = desc;
    }

    RHIUnorderedAccessViewVK::~RHIUnorderedAccessViewVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(mpDevice);
        device->Delete(mImageView);
        device->FreeResourceDescriptor(mHeapIndex);
    }

    bool RHIUnorderedAccessViewVK::Create()
    {
        vk::Device deviceHandle = static_cast<RHIDeviceVK *>(mpDevice)->GetDevice();
        auto dynamicLoader = static_cast<RHIDeviceVK *>(mpDevice)->GetDynamicLoader();
        const vk::PhysicalDeviceDescriptorBufferPropertiesEXT& descBufferProps = static_cast<RHIDeviceVK *>(mpDevice)->GetDescriptorBufferProperties();

        size_t descSize = 0;
        vk::DescriptorGetInfoEXT descGetInfo {};
        vk::DescriptorAddressInfoEXT descBufferInfo {};
        vk::DescriptorImageInfo descImageInfo {};
        descImageInfo.imageLayout = vk::ImageLayout::eGeneral;

        vk::ImageViewUsageCreateInfo imageViewUsageCI {};
        imageViewUsageCI.setUsage(vk::ImageUsageFlagBits::eStorage);

        vk::ImageViewCreateInfo imageViewCI {};
        if (mResource && mResource->IsTexture())
        {
            const RHITextureDesc& textureDesc = static_cast<RHITexture*>(mResource)->GetDesc();
            assert(textureDesc.Usage & RHITextureUsageUnorderedAccess);

            vk::ImageSubresourceRange subresourceRange {};
            subresourceRange.aspectMask = GetAspectFlags(mDesc.Format);
            subresourceRange.baseMipLevel = mDesc.Texture.MipSlice;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = mDesc.Texture.ArraySlice;
            subresourceRange.layerCount = mDesc.Texture.ArraySize;

            imageViewCI.setPNext(&imageViewUsageCI);
            imageViewCI.setImage((VkImage)mResource->GetNativeHandle());
            imageViewCI.setFormat(ToVulkanFormat(mDesc.Format));
            imageViewCI.setSubresourceRange(subresourceRange);

            descGetInfo.type = vk::DescriptorType::eStorageImage;
            descGetInfo.data.pStorageImage = &descImageInfo;
            descSize = descBufferProps.storageImageDescriptorSize;
        }

        switch (mDesc.Type)
        {
        case ERHIUnorderedAccessViewType::Texture2D:
        {
            imageViewCI.setViewType(vk::ImageViewType::e2D);
            mImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(mImageView);
            break;
        }
        case ERHIUnorderedAccessViewType::Texture2DArray:
        {
            imageViewCI.setViewType(vk::ImageViewType::e2DArray);
            mImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(mImageView);
            break;
        }
        case ERHIUnorderedAccessViewType::Texture3D:
        {
            imageViewCI.setViewType(vk::ImageViewType::e3D);
            mImageView = deviceHandle.createImageView(imageViewCI);
            descImageInfo.setImageView(mImageView);
            break;
        }
        case ERHIUnorderedAccessViewType::StructuredBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(mResource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageStructuredBuffer);
            assert(bufferDesc.Usage & RHIBufferUsageUnorderedAccess);
            assert(mDesc.Format == ERHIFormat::Unknown);
            assert(mDesc.Buffer.Offset % bufferDesc.Stride == 0);
            assert(mDesc.Buffer.Size % bufferDesc.Stride == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(mResource)->GetGPUAddress() + mDesc.Buffer.Offset;
            descBufferInfo.range = mDesc.Buffer.Size;

            descGetInfo.setType(vk::DescriptorType::eStorageBuffer);
            descGetInfo.data.pStorageBuffer = &descBufferInfo;
            descSize = descBufferProps.robustStorageBufferDescriptorSize;
            break;
        }
        case ERHIUnorderedAccessViewType::TypedBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(mResource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageTypedBuffer);
            assert(bufferDesc.Usage & RHIBufferUsageUnorderedAccess);
            assert(mDesc.Buffer.Offset % bufferDesc.Stride == 0);
            assert(mDesc.Buffer.Size % bufferDesc.Stride == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(mResource)->GetGPUAddress() + mDesc.Buffer.Offset;
            descBufferInfo.range = mDesc.Buffer.Size;
            descBufferInfo.format = ToVulkanFormat(mDesc.Format);

            descGetInfo.setType(vk::DescriptorType::eStorageTexelBuffer);
            descGetInfo.data.pUniformTexelBuffer = &descBufferInfo;
            descSize = descBufferProps.robustUniformTexelBufferDescriptorSize;
            break;
        }
        case ERHIUnorderedAccessViewType::RawBuffer:
        {
            const RHIBufferDesc& bufferDesc = static_cast<RHIBuffer*>(mResource)->GetDesc();
            assert(bufferDesc.Usage & RHIBufferUsageRawBuffer);
            assert(bufferDesc.Usage & RHIBufferUsageUnorderedAccess);
            assert(bufferDesc.Stride % 4 == 0);
            assert(mDesc.Buffer.Offset % 4 == 0);
            assert(mDesc.Buffer.Size % 4 == 0);

            descBufferInfo.address = static_cast<RHIBuffer*>(mResource)->GetGPUAddress() + mDesc.Buffer.Offset;
            descBufferInfo.range = mDesc.Buffer.Size;

            descGetInfo.setType(vk::DescriptorType::eStorageBuffer);
            descGetInfo.data.pStorageBuffer = &descBufferInfo;
            descSize = descBufferProps.robustStorageBufferDescriptorSize;
            break;
        }
        default:
            break;
        }

        void* pDescriptor = nullptr;
        mHeapIndex = static_cast<RHIDeviceVK *>(mpDevice)->AllocateResourceDescriptor(&pDescriptor);

        deviceHandle.getDescriptorEXT(descGetInfo, descSize, pDescriptor, dynamicLoader);
        return true;
    }

    RHIConstantBufferViewVK::RHIConstantBufferViewVK(RHIDeviceVK *device, RHIBuffer *buffer, const RHIConstantBufferViewDesc &desc, const eastl::string &name)
    {
        mpDevice = device;
        mName = name;
        mBuffer = buffer;
        mDesc = desc;
    }

    RHIConstantBufferViewVK::~RHIConstantBufferViewVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(mpDevice);
        device->FreeResourceDescriptor(mHeapIndex);
    }

    bool RHIConstantBufferViewVK::Create()
    {
        vk::DescriptorAddressInfoEXT descBufferInfo {};
        descBufferInfo.address = mBuffer->GetGPUAddress() + mDesc.Offset;
        descBufferInfo.range = mDesc.Size;

        vk::DescriptorGetInfoEXT descGetInfo {};
        descGetInfo.setType(vk::DescriptorType::eUniformBuffer);
        descGetInfo.setData(&descBufferInfo);

        void* pDescriptor = nullptr;
        mHeapIndex = static_cast<RHIDeviceVK *>(mpDevice)->AllocateResourceDescriptor(&pDescriptor);

        vk::Device deviceHandle = static_cast<RHIDeviceVK *>(mpDevice)->GetDevice();
        auto dynamicLoader = static_cast<RHIDeviceVK *>(mpDevice)->GetDynamicLoader();
        size_t size = static_cast<RHIDeviceVK *>(mpDevice)->GetDescriptorBufferProperties().robustUniformBufferDescriptorSize;

        deviceHandle.getDescriptorEXT(descGetInfo, size, pDescriptor, dynamicLoader);

        return true;
    }

    RHISamplerVK::RHISamplerVK(RHIDeviceVK *device, const RHISamplerDesc &desc, const eastl::string &name)
    {
        mpDevice = device;
        mName = name;
        mDesc = desc;
    }

    RHISamplerVK::~RHISamplerVK()
    {
        RHIDeviceVK *device = static_cast<RHIDeviceVK *>(mpDevice);
        device->Delete(mSampler);
        device->FreeSamplerDescriptor(mHeapIndex);
    }

    bool RHISamplerVK::Create()
    {
        vk::SamplerReductionModeCreateInfo samplerReductionModeCI {};
        samplerReductionModeCI.setReductionMode(ToVKSamplerReductionMode(mDesc.ReductionMode));

        vk::SamplerCreateInfo samplerCI {};
        samplerCI.setPNext(&samplerReductionModeCI);
        samplerCI.setMagFilter(ToVKFilter(mDesc.MagFilter));
        samplerCI.setMinFilter(ToVKFilter(mDesc.MinFilter));
        samplerCI.setMipmapMode(ToVKSamplerMipmapMode(mDesc.MipFilter));
        samplerCI.setAddressModeU(ToVKSamplerAddressMode(mDesc.AddressU));
        samplerCI.setAddressModeV(ToVKSamplerAddressMode(mDesc.AddressV));
        samplerCI.setAddressModeW(ToVKSamplerAddressMode(mDesc.AddressW));
        samplerCI.setMipLodBias(mDesc.MipLODBias);
        samplerCI.setAnisotropyEnable(mDesc.bEnableAnisotropy);
        samplerCI.setMaxAnisotropy(mDesc.MaxAnisotropy);
        samplerCI.setCompareEnable(mDesc.ReductionMode == ERHISamplerReductionMode::Compare);
        samplerCI.setMinLod(mDesc.MinLOD);
        samplerCI.setMaxLod(mDesc.MaxLOD);

        if (mDesc.BorderColor[0] == 0.0f && mDesc.BorderColor[1] == 0.0f && mDesc.BorderColor[2] == 0.0f && mDesc.BorderColor[3] == 0.0f)
        {
            samplerCI.setBorderColor(vk::BorderColor::eFloatTransparentBlack);
        }
        else if (mDesc.BorderColor[0] == 0.0f && mDesc.BorderColor[1] == 0.0f && mDesc.BorderColor[2] == 0.0f && mDesc.BorderColor[3] == 1.0f)
        {
            samplerCI.setBorderColor(vk::BorderColor::eFloatOpaqueBlack);
        }
        else if (mDesc.BorderColor[0] == 1.0f && mDesc.BorderColor[1] == 1.0f && mDesc.BorderColor[2] == 1.0f && mDesc.BorderColor[3] == 1.0f)
        {
            samplerCI.setBorderColor(vk::BorderColor::eFloatOpaqueWhite);
        }
        else
        {
            assert(false);
        }

        vk::Device deviceHandle = static_cast<RHIDeviceVK *>(mpDevice)->GetDevice();
        auto dynamicLoader = static_cast<RHIDeviceVK *>(mpDevice)->GetDynamicLoader();
        mSampler = deviceHandle.createSampler(samplerCI);
        if (!mSampler)
        {
            VTNA_LOG_ERROR("[RHISamplerVK] Failed to create {}", mName);
            return false;
        }

        void* pDescriptor = nullptr;
        mHeapIndex = static_cast<RHIDeviceVK *>(mpDevice)->AllocateSamplerDescriptor(&pDescriptor);

        vk::DescriptorGetInfoEXT descGetInfo {};
        descGetInfo.setType(vk::DescriptorType::eSampler);
        descGetInfo.data.pSampler = &mSampler;

        size_t size = static_cast<RHIDeviceVK *>(mpDevice)->GetDescriptorBufferProperties().samplerDescriptorSize;
        deviceHandle.getDescriptorEXT(descGetInfo, size, pDescriptor, dynamicLoader);

        return true;
    }
}