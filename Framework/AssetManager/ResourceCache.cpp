#include "ResourceCache.hpp"
#include "Core/VultanaEngine.hpp"

namespace Assets
{
    ResourceCache *ResourceCache::GetInstance()
    {
        
        static ResourceCache instance;
        return &instance;
    }

    RenderResources::Texture2D *ResourceCache::GetTexture2D(const std::string &file, bool srgb)
    {
        auto iter = mCachedTexture2D.find(file);
        if (iter != mCachedTexture2D.end())
        {
            iter->second.RefCount++;
            return (RenderResources::Texture2D*)iter->second.Data;
        }
        auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();
        Resource texture;
        texture.RefCount = 1;
        texture.Data = pRenderer->CreateTexture2D(file, srgb);
        mCachedTexture2D.insert({file, texture});

        return (RenderResources::Texture2D*)texture.Data;
    }

    void ResourceCache::ReleaseTexture2D(RenderResources::Texture2D *texture)
    {
        if (texture == nullptr)
        {
            return;
        }
        for (auto iter = mCachedTexture2D.begin(); iter != mCachedTexture2D.end(); iter++)
        {
            if (iter->second.Data == texture)
            {
                iter->second.RefCount--;
                if (iter->second.RefCount == 0)
                {
                    delete texture;
                    mCachedTexture2D.erase(iter);
                }
                return;
            }
        }
        assert(false);
    }
}