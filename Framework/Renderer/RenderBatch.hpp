#pragma once

#include "RHI/RHI.hpp"
#include "Scene/Camera.hpp"
#include "Utilities/LinearAllocator.hpp"

#include <functional>

#define MAX_RENDER_BATCH_CB_COUNT RHI::RHI_MAX_CBV_BINDING

namespace Renderer
{
    using GraphicBatch = std::function<void(RHI::RHICommandList*, const Scene::Camera*)>;

    struct RenderBatch
    {
        RenderBatch(LinearAllocator& cbAllocator) : mCBAllocator(cbAllocator)
        {
            IndexBuffer = nullptr;
        }

        const char* Label = "";
        RHI::RHIPipelineState* PSO = nullptr;

        struct
        {
            void* Data = nullptr;
            uint32_t DataSize = 0;
        } Cb[MAX_RENDER_BATCH_CB_COUNT];

        union
        {
            struct
            {
                RHI::RHIBuffer* IndexBuffer;
                RHI::ERHIFormat IndexFormat;
                uint32_t IndexOffset;
                uint32_t IndexCount;
            };
            struct
            {
                uint32_t DispatchX;
                uint32_t DispatchY;
                uint32_t DispatchZ;
            };
        };
        float3 Center;
        float Radius = 0.0f;
        uint32_t InstanceIndex = 0;
        uint32_t VertexCount = 0;

        void SetPipelineState(RHI::RHIPipelineState* pso)
        {
            PSO = pso;
        }

        void SetConstantBuffer(uint32_t slot, void* data, uint32_t size)
        {
            assert(slot < MAX_RENDER_BATCH_CB_COUNT);
            if (Cb[slot].Data == nullptr || Cb[slot].DataSize < size)
            {
                Cb[slot].Data = mCBAllocator.Allocate((uint32_t)size);
            }
            
            Cb[slot].DataSize = (uint32_t)size;
            memcpy(Cb[slot].Data, data, size);
        }

        void SetIndexBuffer(RHI::RHIBuffer* buffer, RHI::ERHIFormat format, uint32_t offset)
        {
            IndexBuffer = buffer;
            IndexFormat = format;
            IndexOffset = offset;
        }

        void DrawIndexed(uint32_t count)
        {
            IndexCount = count;
        }

        void Draw(uint32_t count)
        {
            VertexCount = count;
        }

    private:
        LinearAllocator& mCBAllocator;
    };

    inline void DrawBatch(RHI::RHICommandList* pCmdList, const RenderBatch& batch)
    {
        GPU_EVENT_DEBUG(pCmdList, batch.Label);

        for (int i = 0; i < MAX_RENDER_BATCH_CB_COUNT; i++)
        {
            if (batch.Cb[i].Data != nullptr)
            {
                pCmdList->SetGraphicsConstants(i, batch.Cb[i].Data, batch.Cb[i].DataSize);
            }
        }
        if (batch.IndexBuffer != nullptr)
        {
            pCmdList->SetIndexBuffer(batch.IndexBuffer, batch.IndexOffset, batch.IndexFormat);
            pCmdList->DrawIndexed(batch.IndexCount);
        }
        else
        {
            pCmdList->Draw(batch.VertexCount);
        }
    }

    struct ComputeBatch
    {
        ComputeBatch(LinearAllocator& cbAllocator) : mCBAllocator(cbAllocator)
        {
        }

        const char* Label = "";
        RHI::RHIPipelineState* PSO = nullptr;

        struct
        {
            void* Data = nullptr;
            uint32_t DataSize = 0;
        } Cb[MAX_RENDER_BATCH_CB_COUNT];

        uint32_t DispatchX = 0;
        uint32_t DispatchY = 0;
        uint32_t DispatchZ = 0;

        void SetPipelineState(RHI::RHIPipelineState* pso)
        {
            PSO = pso;
        }

        void SetConstantBuffer(uint32_t slot, void* data, uint32_t size)
        {
            assert(slot < MAX_RENDER_BATCH_CB_COUNT);
            if (Cb[slot].Data == nullptr || Cb[slot].DataSize < size)
            {
                Cb[slot].Data = mCBAllocator.Allocate((uint32_t)size);
            }

            Cb[slot].DataSize = (uint32_t)size;
            memcpy(Cb[slot].Data, data, size);
        }

        void Dispatch(uint32_t x, uint32_t y, uint32_t z)
        {
            DispatchX = x;
            DispatchY = y;
            DispatchZ = z;
        }
    
    private:
        LinearAllocator& mCBAllocator;
    };

    inline void DispatchComputeBatch(RHI::RHICommandList* pCmdList, const ComputeBatch& batch)
    {
        GPU_EVENT_DEBUG(pCmdList, batch.Label);

        for (int i = 0; i < MAX_RENDER_BATCH_CB_COUNT; i++)
        {
            if (batch.Cb[i].Data != nullptr)
            {
                pCmdList->SetComputeConstants(i, batch.Cb[i].Data, batch.Cb[i].DataSize);
            }
        }
        pCmdList->Dispatch(batch.DispatchX, batch.DispatchY, batch.DispatchZ);
    }
}