#pragma once

#include "Renderer/RendererBase.hpp"

#include <EASTL/hash_map.h>

namespace Assets
{
    class ResourceCache
    {
    public:
        static ResourceCache* GetInstance();

        RenderResources::Texture2D* GetTexture2D(const eastl::string& file, bool srgb = true);
        void ReleaseTexture2D(RenderResources::Texture2D* texture);

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