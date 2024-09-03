#pragma once

#include "RHICommonVK.hpp"
#include "RHI/RHITexture.hpp"

namespace RHI
{
    class RHIDeviceVK;

    class RHITextureVK : public RHITexture
    {
    public:
        RHITextureVK(RHIDeviceVK* device, const RHITextureDesc& desc, const std::string& name);
        ~RHITextureVK();

        bool Create();
        bool Create(vk::Image image);
        bool IsSwapchainTexture() const { return mbSwapchainImage; }
        vk::ImageView GetRenderView(uint32_t mipLevel, uint32_t arraySlice);

        virtual void* GetNativeHandle() const override { return mImage; }

        virtual uint32_t GetRequiredStagingBufferSize() const override;
        virtual uint32_t GetRowPitch(uint32_t mipLevel = 0) const override;
        virtual void* GetSharedHandle() const override;

    private:
        vk::Image mImage;
        VmaAllocation mAllocation;
        bool mbSwapchainImage = false;

        std::vector<vk::ImageView> mRenderViews;
    };
}