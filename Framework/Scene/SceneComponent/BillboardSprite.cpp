#include "BillboardSprite.hpp"

namespace Scene
{
    FBillboardSpriteRenderer::FBillboardSpriteRenderer(Renderer::FRendererBase *pRenderer)
    {
        m_pRenderer = pRenderer;
    }

    FBillboardSpriteRenderer::~FBillboardSpriteRenderer()
    {
    }

    void FBillboardSpriteRenderer::AddSprite(const float3 &position, float size, RenderResources::FTexture2D *texture, const float4 &color, uint32_t objectID)
    {
    }

    void FBillboardSpriteRenderer::Render()
    {

    }
}