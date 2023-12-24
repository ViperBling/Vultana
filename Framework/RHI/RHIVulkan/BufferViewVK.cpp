#include "BufferViewVK.hpp"
#include "BufferVK.hpp"

namespace Vultana
{
    static inline bool IsVertexBuffer(RHIBufferViewType usage)
    {
        return (usage & RHIBufferUsageBits::Vertex) != 0;
    }

    static inline bool IsIndexBuffer(RHIBufferUsageFlags usage)
    {
        return (usage & RHIBufferUsageBits::Index) != 0;
    }

    BufferViewVK::BufferViewVK(BufferVK& inBuffer, const BufferViewCreateInfo& createInfo)
        : RHIBufferView(createInfo)
        , mBuffer(inBuffer)
    {
        InitializeBufferAttributes(createInfo);
    }

    BufferViewVK::~BufferViewVK()
    {
        Destroy();
    }

    void BufferViewVK::Destroy()
    {
    }

    void BufferViewVK::InitializeBufferAttributes(const BufferViewCreateInfo& createInfo)
    {
        mOffset = createInfo.Offset;
        mSize = createInfo.Size;
        if (IsIndexBuffer(mBuffer.GetUsages()))
        {
            mFormat = createInfo.Index.Format;
        }
        else
        {

        }
    }
}