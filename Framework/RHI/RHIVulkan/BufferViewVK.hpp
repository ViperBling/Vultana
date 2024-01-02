#pragma once

#include "RHI/RHIBufferView.hpp"

namespace RHI
{
    class BufferVK;
    class DeviceVK;

    class BufferViewVK : public RHIBufferView
    {
    public:
        NOCOPY(BufferViewVK)
        BufferViewVK(BufferVK& inBuffer, const BufferViewCreateInfo& createInfo);
        ~BufferViewVK();
        void Destroy() override;

        size_t GetOffset() const { return mOffset; }
        size_t GetBufferSize() const { return mSize; }
        RHIIndexFormat GetIndexFormat() const { return mFormat; }
        BufferVK& GetBuffer() { return mBuffer; }

    private:
        void InitializeBufferAttributes(const BufferViewCreateInfo& createInfo);

    private:
        BufferVK& mBuffer;
        size_t mOffset;
        size_t mSize;
        RHIIndexFormat mFormat;
    };
} // namespace Vultana
