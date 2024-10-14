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

    private:
        struct Resource
        {
            void* Data;
            uint32_t RefCount;
        };
        struct SceneBuffer
        {

        };
        std::unordered_map<std::string, Resource> mCachedTexture2D;
    };
}