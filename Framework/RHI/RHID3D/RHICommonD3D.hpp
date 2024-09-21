#pragma once

#include "d3d12.h"
#include "dxgi1_6.h"

#include "RHI/RHICommon.hpp"

#include <string>
#include <cassert>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

namespace RHI
{
    struct D3D12Descriptor
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
    };

    inline bool IsNullDescriptor(const D3D12Descriptor& desc)
    {
        return desc.CPUHandle.ptr == 0 && desc.GPUHandle.ptr == 0;
    }

    inline D3D12_HEAP_TYPE GetD3D12HeapType(ERHIMemoryType type)
    {
        switch (type)
        {
        case ERHIMemoryType::GPUOnly:
            return D3D12_HEAP_TYPE_DEFAULT;
        case ERHIMemoryType::CPUOnly:
        case ERHIMemoryType::CPUToGPU:
            return D3D12_HEAP_TYPE_UPLOAD;
        case ERHIMemoryType::GPUToCPU:
            return D3D12_HEAP_TYPE_READBACK;
        default:
            return D3D12_HEAP_TYPE_DEFAULT;
        }
    }

    inline D3D12_BARRIER_SYNC ToD3DBarrierSync(ERHIAccessFlags flags)
    {
        D3D12_BARRIER_SYNC sync = D3D12_BARRIER_SYNC_NONE;
        bool discard = flags & RHIAccessDiscard;
        if (!discard)
        {
            if (flags & RHIAccessClearUAV) sync |= D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW;
        }
        if (flags & RHIAccessPresent)         sync |= D3D12_BARRIER_SYNC_ALL;
        if (flags & RHIAccessRTV)             sync |= D3D12_BARRIER_SYNC_RENDER_TARGET;
        if (flags & RHIAccessMaskDSV)         sync |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;
        if (flags & RHIAccessMaskVS)          sync |= D3D12_BARRIER_SYNC_VERTEX_SHADING;
        if (flags & RHIAccessMaskPS)          sync |= D3D12_BARRIER_SYNC_PIXEL_SHADING;
        if (flags & RHIAccessMaskCS)          sync |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
        if (flags & RHIAccessMaskCopy)        sync |= D3D12_BARRIER_SYNC_COPY;
        if (flags & RHIAccessShadingRate)     sync |= D3D12_BARRIER_SYNC_PIXEL_SHADING;
        if (flags & RHIAccessIndexBuffer)     sync |= D3D12_BARRIER_SYNC_INDEX_INPUT;
        if (flags & RHIAccessIndirectArgs)    sync |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;
        if (flags & RHIAccessMaskAS)          sync |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;

        return sync;
    }

    
}
