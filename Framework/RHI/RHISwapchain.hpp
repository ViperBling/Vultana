#pragma once

#include "RHIResource.hpp"

namespace RHI
{
    class FRHITexture;

    class FRHISwapchain : public FRHIResource
    {
    public:
        virtual ~FRHISwapchain() = default;

        virtual void AcquireNextBackBuffer() = 0;
        virtual FRHITexture* GetBackBuffer() const = 0;
        virtual bool Resize(uint32_t width, uint32_t height) = 0;
        virtual void SetVSyncEnabled(bool enabled) = 0;

        const FRHISwapchainDesc* GetDesc() const { return &m_Desc; }

    protected:
        FRHISwapchainDesc m_Desc {};
    };
}