#pragma once

#include "Utilities/Math.hpp"
#include "RHICommon.hpp"

namespace Vultana
{
    class RHITexture;
    class RHIQueue;
    class RHISurface;

    struct SwapchainCreateInfo
    {
        RHIQueue* PresentQueue;
        RHISurface* Surface;
        uint8_t TextureCount;
        RHIFormat Format;
        Vector2 Extent;
        RHIPresentMode PresentMode;
    };

    class RHISwapchain
    {
    public:
        NOCOPY(RHISwapchain)
        virtual ~RHISwapchain() = default;

        virtual RHITexture* GetTexture(uint8_t index) = 0;
        virtual uint8_t AcquireBackTexture() = 0;

        virtual void Present() = 0;
        virtual void Destroy() = 0;

    protected:
        explicit RHISwapchain(const SwapchainCreateInfo& createInfo) {}
    };
}