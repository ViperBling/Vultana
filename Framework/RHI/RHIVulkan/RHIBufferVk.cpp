#include "RHIBufferVK.hpp"
#include "RHIDeviceVK.hpp"
#include "RHIHeapVK.hpp"

#include "Utilities/Utility.hpp"
#include "Utilities/Log.hpp"

namespace RHI
{
    RHIBufferVK::RHIBufferVK(RHIDeviceVK *device, const RHIBufferDesc &desc, const std::string &name)
    {
        mpDevice = device;
        mDesc = desc;
        mName = name;
    }

    RHIBufferVK::~RHIBufferVK()
    {
        ((RHIDeviceVK*)mpDevice)->Delete(mBuffer);
        ((RHIDeviceVK*)mpDevice)->Delete(mAllocation);
    }

    bool RHIBufferVK::Create()
    {
        vk::Device device = ((RHIDeviceVK*)mpDevice)->GetDevice();
        auto dynamicLoader = ((RHIDeviceVK*)mpDevice)->GetDynamicLoader();
        VmaAllocator allocator = ((RHIDeviceVK*)mpDevice)->GetVmaAllocator();

        vk::BufferCreateInfo bufferCI {};
        bufferCI.size = mDesc.Size;
        bufferCI.sharingMode = vk::SharingMode::eExclusive;
        bufferCI.usage = 
            vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eTransferSrc |
            vk::BufferUsageFlagBits::eIndexBuffer |
            // vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eIndirectBuffer |
            vk::BufferUsageFlagBits::eShaderDeviceAddress;

        if (mDesc.Usage & RHIBufferUsageConstantBuffer)
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
        }
        if (mDesc.Usage & (RHIBufferUsageStructuredBuffer | RHIBufferUsageRawBuffer ))
        {
            bufferCI.usage |= vk::BufferUsageFlagBits::eStorageBuffer;
        }
        if (mDesc.Usage & RHIBufferUsageTypedBuffer)
        {
            if (mDesc.Usage & RHIBufferUsageUnorderedAccess)
            {
                bufferCI.usage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
            }
            else
            {
                bufferCI.usage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
            }
        }

        vk::Result result;
        if (mDesc.Heap != nullptr)
        {
            assert(mDesc.AllocationType == ERHIAllocationType::Placed);
            assert(mDesc.MemoryType == mDesc.Heap->GetDesc().MemoryType);
            assert(mDesc.Size + mDesc.HeapOffset <= mDesc.Heap->GetDesc().Size);

            result = (vk::Result)vmaCreateAliasingBuffer2(allocator, (VmaAllocation)mDesc.Heap->GetNativeHandle(), (vk::DeviceSize)mDesc.HeapOffset, (VkBufferCreateInfo*)&bufferCI, (VkBuffer*)(&mBuffer));
        }
        else
        {
            VmaAllocationCreateInfo vmaAllocCI {};
            vmaAllocCI.usage = ToVmaUsage(mDesc.MemoryType);
            if (mDesc.AllocationType == ERHIAllocationType::Committed)
            {
                vmaAllocCI.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            }
            if (mDesc.MemoryType != ERHIMemoryType::GPUOnly)
            {
                vmaAllocCI.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
            }
            VmaAllocationInfo vmaAllocInfo {};
            result = (vk::Result)vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bufferCI, &vmaAllocCI, (VkBuffer*)(&mBuffer), &mAllocation, &vmaAllocInfo);

            mpData = vmaAllocInfo.pMappedData;
        }
        
        if (result != vk::Result::eSuccess)
        {
            VTNA_LOG_ERROR("[RHIBufferVK] Failed to create buffer: {}", mName);
            return false;
        }

        SetDebugName(device, vk::ObjectType::eBuffer, mBuffer, mName.c_str(), dynamicLoader);

        if (mAllocation)
        {
            vmaSetAllocationName(allocator, mAllocation, mName.c_str());
        }
        return true;
    }

    void * RHIBufferVK::GetCPUAddress()
    {
        return mpData;
    }
    
    uint64_t RHIBufferVK::GetGPUAddress()
    {
        vk::BufferDeviceAddressInfo addressInfo {};
        addressInfo.buffer = mBuffer;

        return ((RHIDeviceVK*)mpDevice)->GetDevice().getBufferAddress(addressInfo);
    }

    uint32_t RHIBufferVK::GetRequiredStagingBufferSize() const
    {
        vk::MemoryRequirements memReq {};
        ((RHIDeviceVK*)mpDevice)->GetDevice().getBufferMemoryRequirements(mBuffer, &memReq);
        return (uint32_t)memReq.size;
    }
}