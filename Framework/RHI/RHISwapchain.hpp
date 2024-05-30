#pragma once

#include "Utilities/Math.hpp"
#include "RHICommon.hpp"

namespace RHI
{
    class RHITexture;
    class RHIQueue;
    class RHIFence;
    class RHISurface;

    struct SwapchainCreateInfo
    {
        RHIQueue* PresentQueue;
        RHISurface* Surface;
        uint8_t TextureCount;
        RHIFormat Format;
        Math::Vector2 Extent;
        RHIPresentMode PresentMode;

        SwapchainCreateInfo()
            : PresentQueue(nullptr)
            , Surface(nullptr)
            , TextureCount(2)
            , Format(RHIFormat::Count)
            , Extent({ 1, 1 })
            , PresentMode(RHIPresentMode::Immediate)
        {}
        SwapchainCreateInfo& SetPresentQueue(RHIQueue* inPresentQueue)
        {
            PresentQueue = inPresentQueue;
            return *this;
        }
        SwapchainCreateInfo& SetSurface(RHISurface* inSurface)
        {
            Surface = inSurface;
            return *this;
        }
        SwapchainCreateInfo& SetTextureCount(uint8_t inTextureCount)
        {
            TextureCount = inTextureCount;
            return *this;
        }
        SwapchainCreateInfo& SetFormat(RHIFormat inFormat)
        {
            Format = inFormat;
            return *this;
        }
        SwapchainCreateInfo& SetExtent(Math::Vector2 inExtent)
        {
            Extent = inExtent;
            return *this;
        }
        SwapchainCreateInfo& SetPresentMode(RHIPresentMode inPresentMode)
        {
            PresentMode = inPresentMode;
            return *this;
        }
    };

    class RHISwapchain
    {
    public:
        NOCOPY(RHISwapchain)
        virtual ~RHISwapchain() = default;

        virtual RHITexture* GetTexture(uint8_t index) = 0;
        virtual uint8_t AcquireBackTexture() = 0;
        virtual void Present() = 0;

    protected:
        explicit RHISwapchain(const SwapchainCreateInfo& createInfo) {}
    };
}