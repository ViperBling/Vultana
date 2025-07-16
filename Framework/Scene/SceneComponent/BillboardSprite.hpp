#pragma once

#include "Renderer/RendererBase.hpp"

namespace Scene
{
    class BillboardSpriteRenderer
    {
    public:
        BillboardSpriteRenderer(Renderer::RendererBase* pRenderer);
        ~BillboardSpriteRenderer();

        void AddSprite(const float3& position, float size, RenderResources::Texture2D* texture, const float4& color, uint32_t objectID);
        void Render();
    
    private:
        Renderer::RendererBase* m_pRenderer = nullptr;
        RHI::RHIPipelineState* m_pSpritePSO = nullptr;
        RHI::RHIPipelineState* m_pSpriteObjectIDPSO = nullptr;

        struct Sprite
        {
            float3 Position;
            float Size;

            uint32_t Color;
            uint32_t Texture;
            uint32_t ObjectID;
            float Distance;
        };

        eastl::vector<Sprite*> m_Sprites;
    };
}
