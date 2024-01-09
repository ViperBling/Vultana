#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHITexture;

    class RHISwapchain : public RHIResource
    {
    public:
        virtual ~RHISwapchain() = default;

        virtual bool Present() = 0;
        virtual bool Resize(uint32_t width, uint32_t height) = 0;
        virtual void SetVSyncEnabled(bool enable) = 0;

        const RHISwapchainDesc& GetDesc() const { return mDesc; }
        virtual RHITexture* GetBackBuffer() const = 0;

    protected:
        RHISwapchainDesc mDesc {};
    };
}