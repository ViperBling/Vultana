#pragma once

#include "RHIResource.hpp"

namespace Vultana
{
    class RHIFence;
    class RHIPipelineState;

    class RHICommandList : public RHIResource
    {
    public:
        virtual ~RHICommandList() = default;

        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void Wait(RHIFence* fence, uint64_t value) = 0;
        virtual void Signal(RHIFence* fence, uint64_t value) = 0;
        virtual void Submit() = 0;
        virtual void ClearState() = 0;

        virtual void SetPipelineState(RHIPipelineState* pipeline) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t indexOffset = 0) = 0;
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) = 0;
    };
} // namespace Vultana
