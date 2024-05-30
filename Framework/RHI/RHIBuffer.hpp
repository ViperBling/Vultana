#pragma once

#include "RHICommon.hpp"

#include <string>

namespace RHI
{
    struct BufferViewCreateInfo;
    class RHIBufferView;

    struct BufferCreateInfo
    {
        uint32_t Size;
        RHIBufferUsageFlags Usage;
        RHIBufferState InitState;
        std::string Name;

        BufferCreateInfo() = default;
        BufferCreateInfo(uint32_t inSize, RHIBufferUsageFlags inUsage, RHIBufferState inInitState, std::string inName = "")
            : Size(inSize)
            , Usage(inUsage)
            , InitState(inInitState)
            , Name(std::move(inName)) 
        {}
        BufferCreateInfo& SetSize(uint32_t inSize)
        {
            Size = inSize;
            return *this;
        }
        BufferCreateInfo& SetUsage(RHIBufferUsageFlags inUsage)
        {
            Usage = inUsage;
            return *this;
        }
        BufferCreateInfo& SetInitState(RHIBufferState inInitState)
        {
            InitState = inInitState;
            return *this;
        }
        BufferCreateInfo& SetName(std::string inName)
        {
            Name = std::move(inName);
            return *this;
        }

        bool operator==(const BufferCreateInfo& other) const
        {
            return Size == other.Size && Usage == other.Usage;
        }
    };

    class RHIBuffer
    {
    public:
        NOCOPY(RHIBuffer)
        virtual ~RHIBuffer() = default;

        const BufferCreateInfo& GetCreateInfo() const { return mCreateInfo; }
        virtual void* Map(RHIMapMode mapMode, size_t offset = 0, size_t size = 0) = 0;
        virtual void Unmap() = 0;
        virtual std::unique_ptr<RHIBufferView> CreateBufferView(const BufferViewCreateInfo& createInfo) = 0;

    protected:
        explicit RHIBuffer(const BufferCreateInfo& createInfo) 
            : mCreateInfo(createInfo)
        {}

        BufferCreateInfo mCreateInfo;
    };
}
