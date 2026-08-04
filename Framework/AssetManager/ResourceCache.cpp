#include "ResourceCache.hpp"
#include "Core/VultanaEngine.hpp"

namespace Assets
{
    FResourceCache *FResourceCache::GetInstance()
    {
        static FResourceCache instance;
        return &instance;
    }

    RenderResources::FTexture2D *FResourceCache::GetTexture2D(const eastl::string &file, bool srgb)
    {
        auto iter = m_CachedTexture2D.find(file);
        if (iter != m_CachedTexture2D.end())
        {
            iter->second.RefCount++;
            return (RenderResources::FTexture2D*)iter->second.Data;
        }
        auto pRenderer = Core::FVultanaEngine::GetEngineInstance()->GetRenderer();
        FResource texture;
        texture.RefCount = 1;
        texture.Data = pRenderer->CreateTexture2D(file, srgb);
        m_CachedTexture2D.insert(eastl::make_pair(file, texture));

        return (RenderResources::FTexture2D*)texture.Data;
    }

    void FResourceCache::ReleaseTexture2D(RenderResources::FTexture2D *texture)
    {
        if (texture == nullptr)
        {
            return;
        }
        for (auto iter = m_CachedTexture2D.begin(); iter != m_CachedTexture2D.end(); iter++)
        {
            if (iter->second.Data == texture)
            {
                iter->second.RefCount--;
                if (iter->second.RefCount == 0)
                {
                    delete texture;
                    m_CachedTexture2D.erase(iter);
                }
                return;
            }
        }
        assert(false);
    }

    OffsetAllocator::Allocation FResourceCache::GetSceneBuffer(const eastl::string &name, const void *data, uint32_t size)
    {
        auto iter = m_CachedSceneBuffer.find(name);
        if (iter != m_CachedSceneBuffer.end())
        {
            iter->second.RefCount++;
            return iter->second.Allocation;
        }
        auto pRenderer = Core::FVultanaEngine::GetEngineInstance()->GetRenderer();

        FSceneBuffer buffer;
        buffer.RefCount = 1;
        buffer.Allocation = pRenderer->AllocateSceneStaticBuffer(data, size);
        m_CachedSceneBuffer.insert(eastl::make_pair(name, buffer));
        return buffer.Allocation;
    }

    void FResourceCache::ReleaseSceneBuffer(OffsetAllocator::Allocation allocation)
    {
        if (allocation.metadata == OffsetAllocator::Allocation::NO_SPACE)
        {
            return;
        }
        for (auto iter = m_CachedSceneBuffer.begin(); iter != m_CachedSceneBuffer.end(); iter++)
        {
            if (iter->second.Allocation.metadata == allocation.metadata &&
                iter->second.Allocation.offset == allocation.offset)
            {
                iter->second.RefCount--;
                if (iter->second.RefCount == 0)
                {
                    auto pRenderer = Core::FVultanaEngine::GetEngineInstance()->GetRenderer();
                    pRenderer->FreeSceneStaticBuffer(allocation);
                    m_CachedSceneBuffer.erase(iter);
                }
                return;
            }
        }
        assert(false);
    }
}