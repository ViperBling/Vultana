#pragma once

#include "Renderer/RenderResources/RawBuffer.hpp"
#include "Utilities/Math.hpp"
#include <OffsetAllocator/OffsetAllocator.hpp>
#include "Common/GPUScene.hlsli"

namespace Renderer
{
    class RendererBase;

    class GPUScene
    {
    public:
        GPUScene(RendererBase* pRenderer);
        ~GPUScene();

        OffsetAllocator::Allocation AllocateStaticBuffer(uint32_t size);
        void FreeStaticBuffer(OffsetAllocator::Allocation allocation);

        uint32_t AllocateConstantBuffer(uint32_t size);

        uint32_t AddInstance(const FInstanceData& instanceData);
        uint32_t GetInstanceCount() const { return (uint32_t)mInstanceData.size(); }

        void Update();
        void ResetFrameData();

        RHI::RHIBuffer* GetSceneStaticBuffer() const { return mpSceneStaticBuffer->GetBuffer(); }
        RHI::RHIDescriptor* GetSceneStaticBufferSRV() const { return mpSceneStaticBuffer->GetSRV(); }

        RHI::RHIBuffer* GetSceneConstantBuffer() const;
        RHI::RHIDescriptor* GetSceneConstantBufferSRV() const;

        uint32_t GetInstanceDataAddress() const { return mInstanceDataAddress; }

    private:
        RendererBase* mpRenderer = nullptr;

        std::vector<FInstanceData> mInstanceData;
        uint32_t mInstanceDataAddress = 0;

        std::unique_ptr<RenderResources::RawBuffer> mpSceneStaticBuffer;
        std::unique_ptr<OffsetAllocator::Allocator> mpSceneStaticBufferAllocator;

        std::unique_ptr<RenderResources::RawBuffer> mpSceneConstantBuffers[RHI::RHI_MAX_INFLIGHT_FRAMES];
        uint32_t mConstantBufferOffset = 0;
    };
}