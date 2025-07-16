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

        OffsetAllocator::Allocation AllocateAnimationBuffer(uint32_t size);
        void FreeAnimationBuffer(OffsetAllocator::Allocation allocation);

        uint32_t AllocateConstantBuffer(uint32_t size);

        uint32_t AddInstance(const FInstanceData& instanceData);
        uint32_t GetInstanceCount() const { return (uint32_t)m_InstanceData.size(); }

        void Update();
        void ResetFrameData();

        void BeginAnimationUpdate(RHI::RHICommandList* pCmdList);
        void EndAnimationUpdate(RHI::RHICommandList* pCmdList);

        RHI::RHIBuffer* GetSceneStaticBuffer() const { return m_pSceneStaticBuffer->GetBuffer(); }
        RHI::RHIDescriptor* GetSceneStaticBufferSRV() const { return m_pSceneStaticBuffer->GetSRV(); }

        RHI::RHIBuffer* GetSceneAnimationBuffer() const { return m_pSceneAnimationBuffer->GetBuffer(); }
        RHI::RHIDescriptor* GetSceneAnimationBufferSRV() const { return m_pSceneAnimationBuffer->GetSRV(); }
        RHI::RHIDescriptor* GetSceneAnimationBufferUAV() const { return m_pSceneAnimationBuffer->GetUAV(); }

        RHI::RHIBuffer* GetSceneConstantBuffer() const;
        RHI::RHIDescriptor* GetSceneConstantBufferSRV() const;

        uint32_t GetInstanceDataAddress() const { return m_InstanceDataAddress; }

    private:
        RendererBase* m_pRenderer = nullptr;

        eastl::vector<FInstanceData> m_InstanceData;
        uint32_t m_InstanceDataAddress = 0;

        eastl::unique_ptr<RenderResources::RawBuffer> m_pSceneStaticBuffer;
        eastl::unique_ptr<OffsetAllocator::Allocator> m_pSceneStaticBufferAllocator;

        eastl::unique_ptr<RenderResources::RawBuffer> m_pSceneAnimationBuffer;
        eastl::unique_ptr<OffsetAllocator::Allocator> m_pSceneAnimationBufferAllocator;

        eastl::unique_ptr<RenderResources::RawBuffer> m_pSceneConstantBuffers[RHI::RHI_MAX_INFLIGHT_FRAMES];
        uint32_t m_ConstantBufferOffset = 0;
    };
}