#pragma once

#include "RHIResource.hpp"

namespace Vultana
{
    struct BufferViewCreateInfo;
    class RHIBufferView;

    struct BufferCreateInfo
    {
        uint32_t Size;
        RHIBufferUsageFlags Usage;
        std::string Name;
    };

    class RHIBuffer
    {
    public:
        NOCOPY(RHIBuffer)
        virtual ~RHIBuffer() = default;

        virtual void* Map(RHIMapMode mapMode, size_t offset = 0, size_t size = 0) = 0;
        virtual void Unmap() = 0;
        virtual RHIBufferView* CreateBufferView(const BufferViewCreateInfo& createInfo) = 0;
        virtual void Destroy() = 0;

    protected:
        explicit RHIBuffer(const BufferCreateInfo& createInfo) {}
    };
}
