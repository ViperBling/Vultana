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

    template <typename T>
    struct BufferViewCreateInfoBase
    {
        RHIBufferViewType Type;
        uint32_t Offset;
        uint32_t Size;
        std::variant<VertexBufferViewInfo, IndexBufferViewInfo> ViewInfo;

        BufferViewCreateInfoBase() = default;

        T& SetType(RHIBufferViewType inType)
        {
            Type = inType;
            return *static_cast<T*>(this);
        }
        T& SetOffset(uint32_t inOffset)
        {
            Offset = inOffset;
            return *static_cast<T*>(this);
        }
        T& SetSize(uint32_t inSize)
        {
            Size = inSize;
            return *static_cast<T*>(this);
        }
        T& SetViewInfoVertex(uint32_t inStride)
        {
            ViewInfo = VertexBufferViewInfo{ inStride };
            return *static_cast<T*>(this);
        }
        T& SetViewInfoIndex(RHIIndexFormat inFormat)
        {
            ViewInfo = IndexBufferViewInfo{ inFormat };
            return *static_cast<T*>(this);
        }
        size_t Hash() const
        {
            return Utility::HashUtils::CityHash(this, sizeof(BufferCreateInfo));
        }
    };

    struct BufferViewCreateInfo : public BufferViewCreateInfoBase<BufferViewCreateInfo>
    {
        BufferViewCreateInfo() = default;
    };

    class RHIBufferView
    {
    public:
        NOCOPY(RHIBufferView)
        virtual ~RHIBufferView() = default;

    protected:
        explicit RHIBufferView(const BufferViewCreateInfo& createInfo) {}
    };
}