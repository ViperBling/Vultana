#include "BillboardSprite.hpp"

namespace Scene
{
    BillboardSpriteRenderer::BillboardSpriteRenderer(Renderer::RendererBase *pRenderer)
    {
        m_pRenderer = pRenderer;
    }

    BillboardSpriteRenderer::~BillboardSpriteRenderer()
    {
    }

    void BillboardSpriteRenderer::AddSprite(const float3 &position, float size, RenderResources::Texture2D *texture, const float4 &color, uint32_t objectID)
    {
    }

    void BillboardSpriteRenderer::Render()
    {

    }
}