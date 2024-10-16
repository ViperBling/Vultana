#pragma once

#include "Math.hpp"
#include "Memory.hpp"

class LinearAllocator
{
public:
    LinearAllocator(uint32_t memSize)
    {
        mpMemory = VTNA_ALLOC(memSize);
        mMemorySize = memSize;
    }

    ~LinearAllocator()
    {
        VTNA_FREE(mpMemory);
    }

    void* Allocate(uint32_t size, uint32_t alignment = 1)
    {
        uint32_t address = RoundUpPow2(mPointerOffset, alignment);
        assert(address + size <= mMemorySize);
        mPointerOffset = address + size;

        return (char*)mpMemory + address;
    }

    void Reset()
    {
        mPointerOffset = 0;
    }

private:
    void* mpMemory = nullptr;
    uint32_t mMemorySize = 0;
    uint32_t mPointerOffset = 0;
};