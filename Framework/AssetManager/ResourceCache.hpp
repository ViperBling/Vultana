#pragma once

#include "Renderer/RendererBase.hpp"

#include <unordered_map>

namespace Assets
{
    class ResourceCache
    {
    public:
        static ResourceCache* GetInstance();

        RenderResources::Texture2D* GetTexture2D(const std::string& file, bool srgb = true);
        void ReleaseTexture2D(RenderResources::Texture2D* texture);

        OffsetAllocator::Allocation GetSceneBuffer(const std::string& name, const void* data, uint32_t size);
        void ReleaseSceneBuffer(OffsetAllocator::Allocation allocation);

    private:
        struct Resource
        {
            void* Data;
            uint32_t RefCount;
        };
        struct SceneBuffer
        {
            OffsetAllocator::Allocation Allocation;
            uint32_t RefCount;
        };
        std::unordered_map<std::string, Resource> mCachedTexture2D;
        std::unordered_map<std::string, SceneBuffer> mCachedSceneBuffer;
    };
}