#pragma once

#include "RHI/RHI.hpp"

namespace Renderer
{
    void ClearUAV(RHI::FRHICommandList* pCmdList, RHI::FRHIResource* resource, RHI::FRHIDescriptor* descriptor, const RHI::FRHIUnorderedAccessViewDesc& uavDesc, const float* value);
    void ClearUAV(RHI::FRHICommandList* pCmdList, RHI::FRHIResource* resource, RHI::FRHIDescriptor* descriptor, const RHI::FRHIUnorderedAccessViewDesc& uavDesc, const uint32_t* value);
}