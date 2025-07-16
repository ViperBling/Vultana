#pragma once

#include "Math.hpp"
#include "Memory.hpp"

class LinearAllocator
{
public:
    LinearAllocator(uint32_t memSize)
    {
        m_pMemory = VTNA_ALLOC(memSize);
        m_MemorySize = memSize;
    }

    ~LinearAllocator()
    {
        VTNA_FREE(m_pMemory);
    }

    void* Allocate(uint32_t size, uint32_t alignment = 1)
    {
        uint32_t address = RoundUpPow2(m_PointerOffset, alignment);
        assert(address + size <= m_MemorySize);
        m_PointerOffset = address + size;

        return (char*)m_pMemory + address;
    }

    void Reset()
    {
        m_PointerOffset = 0;
    }

private:
    void* m_pMemory = nullptr;
    uint32_t m_MemorySize = 0;
    uint32_t m_PointerOffset = 0;
};