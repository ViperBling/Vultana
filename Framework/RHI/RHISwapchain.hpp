#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class RHITexture;

    class RHISwapchain : public RHIResource
    {
    public:
        virtual ~RHISwapchain() = default;

        virtual void AcquireNextBackBuffer() = 0;
        virtual RHITexture* GetBackBuffer() const = 0;
        virtual bool Resize(uint32_t width, uint32_t height) = 0;
        virtual void SetVSyncEnabled(bool enabled) = 0;

        const RHISwapchainDesc* GetDesc() const { return &m_Desc; }

    protected:
        RHISwapchainDesc m_Desc {};
    };
}