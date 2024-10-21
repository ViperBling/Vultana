#pragma once

#include "RHI/RHI.hpp"

namespace Renderer
{
    void ClearUAV(RHI::RHICommandList* pCmdList, RHI::RHIResource* resource, RHI::RHIDescriptor* descriptor, const RHI::RHIUnorderedAccessViewDesc& uavDesc, const float* value);
    void ClearUAV(RHI::RHICommandList* pCmdList, RHI::RHIResource* resource, RHI::RHIDescriptor* descriptor, const RHI::RHIUnorderedAccessViewDesc& uavDesc, const uint32_t* value);
}