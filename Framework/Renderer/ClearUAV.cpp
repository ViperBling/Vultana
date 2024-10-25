#include "ClearUAV.hpp"
#include "RendererBase.hpp"
#include "Core/VultanaEngine.hpp"

namespace Renderer
{
    static inline eastl::string GetEntryPoint(RHI::ERHIUnorderedAccessViewType uavType)
    {
        switch (uavType)
        {
        case RHI::ERHIUnorderedAccessViewType::Texture2D:
            return "ClearUAV_Texture2D";
        case RHI::ERHIUnorderedAccessViewType::Texture2DArray:
            return "ClearUAV_Texture2DArray";
        case RHI::ERHIUnorderedAccessViewType::Texture3D:
            return "ClearUAV_Texture3D";
        case RHI::ERHIUnorderedAccessViewType::TypedBuffer:
            return "ClearUAV_TypedBuffer";
        case RHI::ERHIUnorderedAccessViewType::RawBuffer:
            return "ClearUAV_RawBuffer";
        default:
            assert(false);
            return "";
        }
    }

    static inline eastl::string GetTypeDefine(RHI::ERHIFormat format)
    {
        switch (format)
        {
        case RHI::ERHIFormat::R32F:
        case RHI::ERHIFormat::R16F:
        case RHI::ERHIFormat::R16UNORM:
        case RHI::ERHIFormat::R8UNORM:
            return "UAV_TYPE_FLOAT";
        case RHI::ERHIFormat::RG32F:
        case RHI::ERHIFormat::RG16F:
        case RHI::ERHIFormat::RG16UNORM:
        case RHI::ERHIFormat::RG8UNORM:
            return "UAV_TYPE_FLOAT2";
        case RHI::ERHIFormat::R11G11B10F:
            return "UAV_TYPE_FLOAT3";
        case RHI::ERHIFormat::RGBA32F:
        case RHI::ERHIFormat::RGBA16F:
        case RHI::ERHIFormat::RGBA16UNORM:
        case RHI::ERHIFormat::RGBA8UNORM:
            return "UAV_TYPE_FLOAT4";
        case RHI::ERHIFormat::R32UI:
        case RHI::ERHIFormat::R16UI:
        case RHI::ERHIFormat::R8UI:
            return "UAV_TYPE_UINT";
        case RHI::ERHIFormat::RG32UI:
        case RHI::ERHIFormat::RG16UI:
        case RHI::ERHIFormat::RG8UI:
            return "UAV_TYPE_UINT2";
        case RHI::ERHIFormat::RGB32UI:
            return "UAV_TYPE_UINT3";
        case RHI::ERHIFormat::RGBA32UI:
        case RHI::ERHIFormat::RGBA16UI:
        case RHI::ERHIFormat::RGBA8UI:
            return "UAV_TYPE_UINT4";
        default:
            assert(false);
            return "";
        }
    }

    template<typename T>
    static inline RHI::RHIShader* GetShader(const RHI::RHIUnorderedAccessViewDesc& uavDesc)
    {
        eastl::string entryPoint = GetEntryPoint(uavDesc.Type);
        eastl::vector<eastl::string> defines;

        if (uavDesc.Type != RHI::ERHIUnorderedAccessViewType::RawBuffer)
        {
            defines.push_back(GetTypeDefine(uavDesc.Format));
        }
        else
        {
            defines.push_back(eastl::is_same<T, float>::value ? "UAV_TYPE_FLOAT4" : "UAV_TYPE_UINT4");
        }
        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        return pRenderer->GetShader("ClearUAV.hlsl", entryPoint, RHI::ERHIShaderType::CS, defines);
    }

    static uint3 GetDispatchGroupCount(RHI::RHIResource* resource, const RHI::RHIUnorderedAccessViewDesc& uavDesc)
    {
        switch (uavDesc.Type)
        {
        case RHI::ERHIUnorderedAccessViewType::Texture2D:
        {
            const RHI::RHITextureDesc& textureDesc = static_cast<RHI::RHITexture*>(resource)->GetDesc();
            return uint3(DivideRoudingUp(textureDesc.Width, 8), DivideRoudingUp(textureDesc.Height, 8), 1);
        }
        case RHI::ERHIUnorderedAccessViewType::Texture2DArray:
        {
            const RHI::RHITextureDesc& textureDesc = static_cast<RHI::RHITexture*>(resource)->GetDesc();
            return uint3(DivideRoudingUp(textureDesc.Width, 8), DivideRoudingUp(textureDesc.Height, 8), textureDesc.ArraySize);
        }
        case RHI::ERHIUnorderedAccessViewType::Texture3D:
        {
            const RHI::RHITextureDesc& textureDesc = static_cast<RHI::RHITexture*>(resource)->GetDesc();
            return uint3(DivideRoudingUp(textureDesc.Width, 8), DivideRoudingUp(textureDesc.Height, 8), DivideRoudingUp(textureDesc.Depth, 8));
        }
        case RHI::ERHIUnorderedAccessViewType::TypedBuffer:
        {
            const RHI::RHIBufferDesc& bufferDesc = static_cast<RHI::RHIBuffer*>(resource)->GetDesc();
            uint32_t elementCount = uavDesc.Buffer.Size / bufferDesc.Stride;
            return uint3(DivideRoudingUp(elementCount, 64), 1, 1);
        }
        case RHI::ERHIUnorderedAccessViewType::RawBuffer:
        {
            uint32_t elementCount = uavDesc.Buffer.Size / 16;
            return uint3(DivideRoudingUp(elementCount, 64), 1, 1);
        }
        default:
            assert(false);
            return uint3(0, 0, 0);
        }
    }

    template<typename T>
    void ClearUAVImpl(RHI::RHICommandList* pCmdList, RHI::RHIResource* resource, RHI::RHIDescriptor* descriptor, const RHI::RHIUnorderedAccessViewDesc& uavDesc, const T* value)
    {
        GPU_EVENT_DEBUG(pCmdList, "ClearUAV");

        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

        RHI::RHIComputePipelineStateDesc psoDesc;
        psoDesc.CS = GetShader<T>(uavDesc);
        pCmdList->SetPipelineState(pRenderer->GetPipelineState(psoDesc, "ClearUAV"));

        uint32_t uavIndex = descriptor->GetHeapIndex();
        pCmdList->SetComputeConstants(0, &uavIndex, sizeof(uint32_t));
        pCmdList->SetComputeConstants(1, value, sizeof(T) * 4);
        
        uint3 groupCount = GetDispatchGroupCount(resource, uavDesc);
        pCmdList->Dispatch(groupCount.x, groupCount.y, groupCount.z);
    }

    void ClearUAV(RHI::RHICommandList *pCmdList, RHI::RHIResource *resource, RHI::RHIDescriptor *descriptor, const RHI::RHIUnorderedAccessViewDesc &uavDesc, const float *value)
    {
        ClearUAVImpl<float>(pCmdList, resource, descriptor, uavDesc, value);
    }

    void ClearUAV(RHI::RHICommandList *pCmdList, RHI::RHIResource *resource, RHI::RHIDescriptor *descriptor, const RHI::RHIUnorderedAccessViewDesc &uavDesc, const uint32_t *value)
    {
        ClearUAVImpl<uint32_t>(pCmdList, resource, descriptor, uavDesc, value);
    }
}