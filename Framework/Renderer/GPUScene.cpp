#include "GPUScene.hpp"
#include "RendererBase.hpp"

#define MAX_CONSTANT_BUFFER_SIZE (1024 * 1024 * 8)
#define ALLOCATION_ALIGNMENT (4)

namespace Renderer
{
    GPUScene::GPUScene(RendererBase *pRenderer)
    {
        mpRenderer = pRenderer;

        const uint32_t staticBufferSize = 1024 * 1024 * 448;
        mpSceneStaticBuffer.reset(pRenderer->CreateRawBuffer(nullptr, staticBufferSize, "GPUScene::StaticBuffer"));
        mpSceneStaticBufferAllocator = std::make_unique<OffsetAllocator::Allocator>(staticBufferSize);

        for (int i = 0; i < RHI::RHI_MAX_INFLIGHT_FRAMES; i++)
        {
            mpSceneConstantBuffers[i].reset(pRenderer->CreateRawBuffer(nullptr, MAX_CONSTANT_BUFFER_SIZE, "GPUScene::ConstantBuffer", RHI::ERHIMemoryType::CPUToGPU, false));
        }
    }

    GPUScene::~GPUScene()
    {
    }

    OffsetAllocator::Allocation GPUScene::AllocateStaticBuffer(uint32_t size)
    {
        return mpSceneStaticBufferAllocator->allocate(RoundUpPow2(size, ALLOCATION_ALIGNMENT));
    }
    
    void GPUScene::FreeStaticBuffer(OffsetAllocator::Allocation allocation)
    {
        if (allocation.offset >= mpSceneStaticBuffer->GetBuffer()->GetDesc().Size)
        {
            return;
        }
        mpSceneStaticBufferAllocator->free(allocation);
    }

    uint32_t GPUScene::AllocateConstantBuffer(uint32_t size)
    {
        assert(mConstantBufferOffset + size <= MAX_CONSTANT_BUFFER_SIZE);
        uint32_t address = mConstantBufferOffset;
        mConstantBufferOffset += RoundUpPow2(size, ALLOCATION_ALIGNMENT);
        return address;
    }

    uint32_t GPUScene::AddInstance(const FInstanceData &instanceData)
    {
        mInstanceData.push_back(instanceData);
        uint32_t instanceID = (uint32_t)mInstanceData.size() - 1;
        return instanceID;
    }

    void GPUScene::Update()
    {
        uint32_t instanceCount = (uint32_t)mInstanceData.size();
        mInstanceDataAddress = mpRenderer->AllocateSceneConstantBuffer(mInstanceData.data(), sizeof(FInstanceData) * instanceCount);
    }

    void GPUScene::ResetFrameData()
    {
        mInstanceData.clear();
        mConstantBufferOffset = 0;
    }

    RHI::RHIBuffer *GPUScene::GetSceneConstantBuffer() const
    {
        uint32_t frameIdx = mpRenderer->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        return mpSceneConstantBuffers[frameIdx]->GetBuffer();
    }

    RHI::RHIDescriptor *GPUScene::GetSceneConstantBufferSRV() const
    {
        uint32_t frameIdx = mpRenderer->GetFrameID() % RHI::RHI_MAX_INFLIGHT_FRAMES;
        return mpSceneConstantBuffers[frameIdx]->GetSRV();
    }
}