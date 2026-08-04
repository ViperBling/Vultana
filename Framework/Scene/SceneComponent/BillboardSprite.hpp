#pragma once

#include "Renderer/RendererBase.hpp"

namespace Scene
{
    class FBillboardSpriteRenderer
    {
    public:
        FBillboardSpriteRenderer(Renderer::FRendererBase* pRenderer);
        ~FBillboardSpriteRenderer();

        void AddSprite(const float3& position, float size, RenderResources::FTexture2D* texture, const float4& color, uint32_t objectID);
        void Render();
    
    private:
        Renderer::FRendererBase* m_pRenderer = nullptr;
        RHI::FRHIPipelineState* m_pSpritePSO = nullptr;
        RHI::FRHIPipelineState* m_pSpriteObjectIDPSO = nullptr;

        struct FSprite
        {
            float3 Position;
            float Size;

            uint32_t Color;
            uint32_t Texture;
            uint32_t ObjectID;
            float Distance;
        };

        eastl::vector<FSprite*> m_Sprites;
    };
}
