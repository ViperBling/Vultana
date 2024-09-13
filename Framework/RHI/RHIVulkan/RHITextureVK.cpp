#include "RHITextureVK.hpp"
#include "RHIDeviceVK.hpp"

#include "Utilities/Log.hpp"

namespace RHI
{
    RHITextureVK::RHITextureVK(RHIDeviceVK *device, const RHITextureDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mDesc = desc;
        mName = name;
    }

    RHITextureVK::~RHITextureVK()
    {
        auto device = (RHIDeviceVK*)mpDevice;
        device->CancelDefaultLayoutTransition(this);

        if (!mbSwapchainImage)
        {
            device->Delete(mImage);
            device->Delete(mAllocation);
        }
        for (size_t i = 0; i < mRenderViews.size(); i++)
        {
            device->Delete(mRenderViews[i]);
        }
    }

    bool RHITextureVK::Create()
    {
        return false;
    }

    bool RHITextureVK::Create(vk::Image image)
    {
        return false;
    }

    vk::ImageView RHITextureVK::GetRenderView(uint32_t mipLevel, uint32_t arraySlice)
    {
        return vk::ImageView();
    }

    uint32_t RHITextureVK::GetRequiredStagingBufferSize() const
    {
        return 0;
    }

    uint32_t RHITextureVK::GetRowPitch(uint32_t mipLevel) const
    {
        return 0;
    }

    void *RHITextureVK::GetSharedHandle() const
    {
        return nullptr;
    }
}