#include "RendererBase.hpp"
#include "ForwardPath/ForwardBasePass.hpp"
#include "Core/VultanaEngine.hpp"

namespace Renderer
{
    void RendererBase::BuildRenderGraph(RG::RGHandle &outputColor, RG::RGHandle &outputDepth)
    {
        mpRenderGraph->Clear();

        mpForwardBasePass->Render(mpRenderGraph.get());

        outputColor = mpForwardBasePass->GetBasePassColorRT();
        outputDepth = mpForwardBasePass->GetBasePassDepthRT();

        mpRenderGraph->Present(outputColor, RHI::RHIAccessPixelShaderSRV);
        mpRenderGraph->Present(outputDepth, RHI::RHIAccessDSVReadOnly);

        mpRenderGraph->Compile();
    }
}