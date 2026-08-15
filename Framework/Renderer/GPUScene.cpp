#include "Renderer/GPUScene.hpp"
#include "Renderer/RendererBase.hpp"

#include "Utilities/Log.hpp"

#define ALLOCATION_ALIGNMENT (4)

namespace Renderer
{
    static_assert(OffsetAllocator::Allocation::NO_SPACE == 0xFFFFFFFFu, "Relies on NO_SPACE matching INVALID_ADDRESS (Scene/ECS/AssetRegistry.hpp)");
    static_assert(PERSISTENT_CONSTANT_REGION_SIZE < NULL_CONSTANT_ADDRESS, "persistent region must fit below the transient/null boundary");

    FGPUScene::FGPUScene(FRendererBase *pRenderer)
    {
        m_pRenderer = pRenderer;

        const uint32_t staticBufferSize = 1024 * 1024 * 512;
        m_pSceneStaticBuffer.reset(pRenderer->CreateRawBuffer(nullptr, staticBufferSize, "GPUScene::StaticBuffer"));
        m_pSceneStaticBufferAllocator = eastl::make_unique<OffsetAllocator::Allocator>(staticBufferSize);

        const uint32_t animationBufferSize = 1024 * 1024 * 32;
        m_pSceneAnimationBuffer.reset(pRenderer->CreateRawBuffer(nullptr, animationBufferSize, "GPUScene::AnimationBuffer", RHI::ERHIMemoryType::GPUOnly, true));
        m_pSceneAnimationBufferAllocator = eastl::make_unique<OffsetAllocator::Allocator>(animationBufferSize);

        for (int i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            m_pSceneConstantBuffers[i].reset(pRenderer->CreateRawBuffer(nullptr, MAX_SCENE_CONSTANT_BUFFER_SIZE, "GPUScene::ConstantBuffer", RHI::ERHIMemoryType::CPUToGPU, false));
            if (m_pSceneConstantBuffers[i])
            {
                memset((char*)m_pSceneConstantBuffers[i]->GetBuffer()->GetCPUAddress() + NULL_CONSTANT_ADDRESS, 0, NULL_CONSTANT_REGION_SIZE);
            }
        }

        m_pPersistentConstantAllocator = eastl::make_unique<OffsetAllocator::Allocator>(PERSISTENT_CONSTANT_REGION_SIZE, 4096);
    }

    FGPUScene::~FGPUScene()
    {
    }

    OffsetAllocator::Allocation FGPUScene::AllocateStaticBuffer(uint32_t size)
    {
        return m_pSceneStaticBufferAllocator->allocate(RoundUpPow2(size, ALLOCATION_ALIGNMENT));
    }
    
    void FGPUScene::FreeStaticBuffer(OffsetAllocator::Allocation allocation)
    {
        if (allocation.offset >= m_pSceneStaticBuffer->GetBuffer()->GetDesc().Size)
        {
            return;
        }
        m_pSceneStaticBufferAllocator->free(allocation);
    }

    OffsetAllocator::Allocation FGPUScene::AllocateAnimationBuffer(uint32_t size)
    {
        return m_pSceneAnimationBufferAllocator->allocate(RoundUpPow2(size, ALLOCATION_ALIGNMENT));
    }

    void FGPUScene::FreeAnimationBuffer(OffsetAllocator::Allocation allocation)
    {
        if (allocation.offset >= m_pSceneAnimationBuffer->GetBuffer()->GetDesc().Size)
        {
            return;
        }
        m_pSceneAnimationBufferAllocator->free(allocation);
    }

    uint32_t FGPUScene::AllocateConstantBuffer(uint32_t size)
    {
        static bool bOverflowWarned = false;
        if (m_ConstantBufferOffset + size > NULL_CONSTANT_ADDRESS)
        {
            if (!bOverflowWarned)
            {
                bOverflowWarned = true;
                VTNA_LOG_ERROR("Constant buffer overflow: offset={} size={} cap={} — degraded to null region", m_ConstantBufferOffset, size, NULL_CONSTANT_ADDRESS);
            }
            return NULL_CONSTANT_ADDRESS;
        }

        uint32_t address = m_ConstantBufferOffset;
        m_ConstantBufferOffset += RoundUpPow2(size, ALLOCATION_ALIGNMENT);
        return address;
    }

    OffsetAllocator::Allocation FGPUScene::AllocatePersistentConstantBuffer(const void* data, uint32_t size)
    {
        OffsetAllocator::Allocation allocation = m_pPersistentConstantAllocator->allocate(RoundUpPow2(size, ALLOCATION_ALIGNMENT));
        if (allocation.offset == OffsetAllocator::Allocation::NO_SPACE)
        {
            static bool bPersistentOverflowWarned = false;
            if (!bPersistentOverflowWarned)
            {
                bPersistentOverflowWarned = true;
                VTNA_LOG_ERROR("Persistent constant buffer overflow: size={} cap={} — degraded to null region", size, PERSISTENT_CONSTANT_REGION_SIZE);
            }
            return {};
        }
        for (int i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; ++i)
        {
            void* dst = (char*)m_pSceneConstantBuffers[i]->GetBuffer()->GetCPUAddress() + allocation.offset;
            memcpy(dst, data, size);
        }
        return allocation;
    }

    void FGPUScene::WritePersistentConstantBuffer(OffsetAllocator::Allocation allocation, const void* data, uint32_t size)
    {
        if (allocation.offset == OffsetAllocator::Allocation::NO_SPACE)
            return;
        for (int i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; ++i)
        {
            void* dst = (char*)m_pSceneConstantBuffers[i]->GetBuffer()->GetCPUAddress() + allocation.offset;
            memcpy(dst, data, size);
        }
    }

    void FGPUScene::FreePersistentConstantBuffer(OffsetAllocator::Allocation allocation)
    {
        if (allocation.offset == OffsetAllocator::Allocation::NO_SPACE)
            return;
        m_pPersistentConstantAllocator->free(allocation);
    }

    uint32_t FGPUScene::AddInstance(const FInstanceData& instanceData)
    {
        m_InstanceData.push_back(instanceData);
        uint32_t instanceID = (uint32_t)m_InstanceData.size() - 1;
        return instanceID;
    }

    void FGPUScene::Update()
    {
        uint32_t instanceCount = (uint32_t)m_InstanceData.size();
        m_InstanceDataAddress = m_pRenderer->AllocateSceneConstantBuffer(m_InstanceData.data(), sizeof(FInstanceData) * instanceCount);
    }

    void FGPUScene::BeginAnimationUpdate(RHI::FRHICommandList *pCmdList)
    {
        pCmdList->BufferBarrier(m_pSceneAnimationBuffer->GetBuffer(), RHI::RHIAccessVertexShaderSRV, RHI::RHIAccessComputeUAV);
    }

    void FGPUScene::EndAnimationUpdate(RHI::FRHICommandList *pCmdList)
    {
        pCmdList->BufferBarrier(m_pSceneAnimationBuffer->GetBuffer(), RHI::RHIAccessComputeUAV, RHI::RHIAccessVertexShaderSRV);
    }

    void FGPUScene::ResetFrameData()
    {
        m_InstanceData.clear();
        m_ConstantBufferOffset = PERSISTENT_CONSTANT_REGION_SIZE;
    }

    RHI::FRHIBuffer *FGPUScene::GetSceneConstantBuffer() const
    {
        uint32_t frameIdx = m_pRenderer->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        return m_pSceneConstantBuffers[frameIdx]->GetBuffer();
    }

    RHI::FRHIDescriptor *FGPUScene::GetSceneConstantBufferSRV() const
    {
        uint32_t frameIdx = m_pRenderer->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        return m_pSceneConstantBuffers[frameIdx]->GetSRV();
    }
}