#include "GPUScene.hpp"
#include "RendererBase.hpp"

#define MAX_CONSTANT_BUFFER_SIZE (1024 * 1024 * 8)
#define ALLOCATION_ALIGNMENT (4)

namespace Renderer
{
    GPUScene::GPUScene(RendererBase *pRenderer)
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
            m_pSceneConstantBuffers[i].reset(pRenderer->CreateRawBuffer(nullptr, MAX_CONSTANT_BUFFER_SIZE, "GPUScene::ConstantBuffer", RHI::ERHIMemoryType::CPUToGPU, false));
        }
    }

    GPUScene::~GPUScene()
    {
    }

    OffsetAllocator::Allocation GPUScene::AllocateStaticBuffer(uint32_t size)
    {
        return m_pSceneStaticBufferAllocator->allocate(RoundUpPow2(size, ALLOCATION_ALIGNMENT));
    }
    
    void GPUScene::FreeStaticBuffer(OffsetAllocator::Allocation allocation)
    {
        if (allocation.offset >= m_pSceneStaticBuffer->GetBuffer()->GetDesc().Size)
        {
            return;
        }
        m_pSceneStaticBufferAllocator->free(allocation);
    }

    OffsetAllocator::Allocation GPUScene::AllocateAnimationBuffer(uint32_t size)
    {
        return m_pSceneAnimationBufferAllocator->allocate(RoundUpPow2(size, ALLOCATION_ALIGNMENT));
    }

    void GPUScene::FreeAnimationBuffer(OffsetAllocator::Allocation allocation)
    {
        if (allocation.offset >= m_pSceneAnimationBuffer->GetBuffer()->GetDesc().Size)
        {
            return;
        }
        m_pSceneAnimationBufferAllocator->free(allocation);
    }

    uint32_t GPUScene::AllocateConstantBuffer(uint32_t size)
    {
        assert(m_ConstantBufferOffset + size <= MAX_CONSTANT_BUFFER_SIZE);
        uint32_t address = m_ConstantBufferOffset;
        m_ConstantBufferOffset += RoundUpPow2(size, ALLOCATION_ALIGNMENT);
        return address;
    }

    uint32_t GPUScene::AddInstance(const FInstanceData &instanceData)
    {
        m_InstanceData.push_back(instanceData);
        uint32_t instanceID = (uint32_t)m_InstanceData.size() - 1;
        return instanceID;
    }

    void GPUScene::Update()
    {
        uint32_t instanceCount = (uint32_t)m_InstanceData.size();
        m_InstanceDataAddress = m_pRenderer->AllocateSceneConstantBuffer(m_InstanceData.data(), sizeof(FInstanceData) * instanceCount);
    }

    void GPUScene::BeginAnimationUpdate(RHI::RHICommandList *pCmdList)
    {
        pCmdList->BufferBarrier(m_pSceneAnimationBuffer->GetBuffer(), RHI::RHIAccessVertexShaderSRV, RHI::RHIAccessComputeUAV);
    }

    void GPUScene::EndAnimationUpdate(RHI::RHICommandList *pCmdList)
    {
        pCmdList->BufferBarrier(m_pSceneAnimationBuffer->GetBuffer(), RHI::RHIAccessComputeUAV, RHI::RHIAccessVertexShaderSRV);
    }

    void GPUScene::ResetFrameData()
    {
        m_InstanceData.clear();
        m_ConstantBufferOffset = 0;
    }

    RHI::RHIBuffer *GPUScene::GetSceneConstantBuffer() const
    {
        uint32_t frameIdx = m_pRenderer->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        return m_pSceneConstantBuffers[frameIdx]->GetBuffer();
    }

    RHI::RHIDescriptor *GPUScene::GetSceneConstantBufferSRV() const
    {
        uint32_t frameIdx = m_pRenderer->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        return m_pSceneConstantBuffers[frameIdx]->GetSRV();
    }
}