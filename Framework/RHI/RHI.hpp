#pragma once

#include "RHICommon.hpp"
#include "RHIBuffer.hpp"
#include "RHICommandList.hpp"
#include "RHIDevice.hpp"
#include "RHIDescriptor.hpp"
#include "RHIFence.hpp"
#include "RHIHeap.hpp"
#include "RHIPipelineState.hpp"
#include "RHIShader.hpp"
#include "RHISwapchain.hpp"
#include "RHITexture.hpp"

namespace RHI
{
    RHIDevice* CreateRHIDevice(const RHIDeviceDesc& desc);
    // Get the row pitch of a format
    uint32_t GetFormatRowPitch(ERHIFormat format, uint32_t width);
    // Get the block size of a compressed format
    uint32_t GetFormatBlockWidth(ERHIFormat format);
    uint32_t GetFormatBlockHeight(ERHIFormat format);
    uint32_t GetFormatComponentNum(ERHIFormat format);
    bool IsDepthFormat(ERHIFormat format);
    bool IsStencilFormat(ERHIFormat format);
    bool IsSRGBFormat(ERHIFormat format);
    // Calculate the subresource index
    uint32_t CalcSubresource(const RHITextureDesc& desc, uint32_t mipLevel, uint32_t arraySlice);
    // Decompose a subresource into mip level and array slice
    void DecomposeSubresource(const RHITextureDesc& desc, uint32_t subresource, uint32_t& mipLevel, uint32_t& arraySlice);

    class RenderEvent
    {
    public:
        RenderEvent(RHICommandList* cmdList, const eastl::string& eventName) : mpCmdList(cmdList)
        {
            mpCmdList->BeginEvent(eventName);
        }

        ~RenderEvent()
        {
            mpCmdList->EndEvent();
        }
    
    private:
        RHICommandList* mpCmdList = nullptr;
    };
}

#define GPU_EVENT_DEBUG(pCmdList, eventName) RHI::RenderEvent __render_event__(pCmdList, eventName)