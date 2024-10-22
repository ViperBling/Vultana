#pragma once

#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/RenderBatch.hpp"

namespace Renderer
{
    class RendererBase;

    class ForwardBasePass
    {
    public:
        ForwardBasePass(RendererBase* pRenderer);
        
        RenderBatch& AddBatch();
        void Render(RG::RenderGraph* pRenderGraph);

        RG::RGHandle GetBasePassColorRT() const { return mBasePassColorRT; }
        RG::RGHandle GetBasePassDepthRT() const { return mBasePassDepthRT; }

    private:
        RendererBase* mpRenderer;
        std::vector<RenderBatch> mInstance;

        RG::RGHandle mBasePassColorRT;
        RG::RGHandle mBasePassDepthRT;
    };
}