#pragma once

#include "RHICommon.hpp"

namespace RHI
{
    struct VertexBufferViewInfo
    {
        uint32_t Stride;
    };

    struct IndexBufferViewInfo
    {
        RHIIndexFormat Format;
    };

    struct BufferViewCreateInfo
    {
        RHIBufferViewType Type;
        uint32_t Offset;
        uint32_t Size;
        union
        {
            VertexBufferViewInfo Vertex;
            IndexBufferViewInfo Index;
        };
    };

    class RHIBufferView
    {
    public:
        NOCOPY(RHIBufferView)
        virtual ~RHIBufferView() = default;

        virtual void Destroy() = 0;

    protected:
        explicit RHIBufferView(const BufferViewCreateInfo& createInfo) {}
    };
}