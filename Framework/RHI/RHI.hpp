#pragma once

#include "RHICommon.hpp"
#include "RHIBuffer.hpp"
#include "RHICommandList.hpp"
#include "RHIDevice.hpp"
#include "RHIFence.hpp"
#include "RHIHeap.hpp"
#include "RHIPipelineState.hpp"
#include "RHIShader.hpp"
#include "RHISwapchain.hpp"
#include "RHITexture.hpp"

namespace RHI
{
    RHIDevice* CreateRHIDevice(const RHIDeviceDesc& desc);
    uint32_t GetFormatRowPitch(ERHIFormat format, uint32_t width);
    uint32_t GetFormatBlockWidth(ERHIFormat format);
    uint32_t GetFormatBlockHeight(ERHIFormat format);
    uint32_t GetFormatComponentNum(ERHIFormat format);
    bool IsDepthFormat(ERHIFormat format);
    bool IsStencilFormat(ERHIFormat format);
}