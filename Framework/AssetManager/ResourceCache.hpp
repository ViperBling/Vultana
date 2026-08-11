#pragma once

#include "AssetCooker/VAssetTypes.hpp"
#include "Renderer/RendererBase.hpp"

#include <EASTL/hash_map.h>

namespace Assets
{
    class FResourceCache
    {
    public:
        static FResourceCache* GetInstance();

        RenderResources::FTexture2D* GetTexture2D(const eastl::string& file, bool srgb = true);
        void ReleaseTexture2D(RenderResources::FTexture2D* texture);

        RenderResources::FTexture2D* GetTexture2DProduct(const eastl::string& productPath, const Assets::FTextureCookedData& payload);
        RenderResources::FTexture2D* GetTexture2DProductReady(const eastl::string& productPath);

        void ReleaseAllTextures();

        OffsetAllocator::Allocation GetSceneBuffer(const eastl::string& name, const void* data, uint32_t size);
        void ReleaseSceneBuffer(OffsetAllocator::Allocation allocation);

    private:
        struct FResource
        {
            void* Data;
            uint32_t RefCount;
        };
        struct FSceneBuffer
        {
            OffsetAllocator::Allocation Allocation;
            uint32_t RefCount;
        };
        eastl::hash_map<eastl::string, FResource> m_CachedTexture2D;
        eastl::hash_map<eastl::string, FSceneBuffer> m_CachedSceneBuffer;
    };
}