#include "AssetManager/AssetManager.hpp"

#include "AssetCooker/VultanaCooker.hpp"
#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetCooker/AssetValidate.hpp"

#include "Core/VultanaEngine.hpp"

#include "Utilities/Log.hpp"

#include <enkiTS/TaskScheduler.h>
#include <sys/stat.h>
#include <EASTL/algorithm.h>
#include <EASTL/utility.h>
#include "AssetManager.hpp"

namespace Assets
{
    class FAssetManager::FAsyncModelLoadTask final : public enki::ITaskSet
    {
    public:
        FAsyncModelLoadTask(FAssetManager* pManager, FRequest request)
            : m_pManager(pManager), m_Request(eastl::move(request)) {}

        void ExecuteRange(enki::TaskSetPartition range, uint32_t threadNum) override
        {
            m_pManager->LoadWorker(eastl::move(m_Request));
        }

    private:
        FAssetManager* m_pManager = nullptr;
        FRequest m_Request;
    };

    FAssetManager::~FAssetManager()
    {
        for (FAsyncModelLoadTask* task : m_Tasks)
            delete task;
        m_Tasks.clear();
    }

    void FAssetManager::RequestModel(const eastl::string& srcPath, FModelLoadCallback&& callback)
    {
        const eastl::string normalized = FAssetPathUtils::NormalizeAssetPath(srcPath);

        FRequest request;
        request.SrcPath = srcPath;
        request.NormalizedPath = normalized;
        request.Callback = eastl::move(callback);

        bool bSpawnWorker = false;
        {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);

            auto cacheIt = m_ModelCache.find(normalized);
            if (cacheIt != m_ModelCache.end())
            {
                // Cache hit
                FReadyResult readyResult;
                readyResult.NormalizedPath = normalized;
                readyResult.Callback = eastl::move(request.Callback);
                readyResult.Result.bSuccess = true;
                readyResult.Result.SrcPath = srcPath;
                readyResult.Result.Header = cacheIt->second.Header;
                readyResult.Result.Payload = cacheIt->second.Payload;
                readyResult.Result.UploadBytes = cacheIt->second.UploadBytes;
                cacheIt->second.RefCount++;
                m_ReadyQueue.push_back(eastl::move(readyResult));
                ++m_PendingCount;
                return;
            }
            if (m_Loading.find(normalized) != m_Loading.end())
            {
                // Already loading, defer callback
                m_PendingCallbacks[normalized].push_back(eastl::move(request.Callback));
                ++m_PendingCount;
                return;
            }
            m_Loading.insert(normalized);
            ++m_PendingCount;
            bSpawnWorker = true;
        }
        if (bSpawnWorker)
        {
            SpawnWorker(eastl::move(request));
        }
    }

    void FAssetManager::DrainReadyQueue()
    {
        eastl::vector<FReadyResult> deferred;
        uint32_t budgetRemaining = kMaxUploadBytesPerFrame;
        bool bProcessedAny = false;

        while (true)
        {
            FReadyResult readyResult;
            {
                std::lock_guard<std::recursive_mutex> lock(m_Mutex);
                if (m_ReadyQueue.empty())
                    break;
                readyResult = eastl::move(m_ReadyQueue.front());
                m_ReadyQueue.pop_front();
            }

            if (readyResult.Result.bSuccess)
            {
                const uint32_t bytes = readyResult.Result.UploadBytes;
                if (bytes > budgetRemaining)
                {
                    deferred.push_back(eastl::move(readyResult));
                    continue;
                }
                budgetRemaining -= bytes;
                bProcessedAny = true;
            }
            FinishReadyResult(eastl::move(readyResult));
        }
        if (!deferred.empty())
        {
            if (!bProcessedAny || deferred.front().Result.UploadBytes > kMaxUploadBytesPerFrame)
            {
                FReadyResult first = eastl::move(deferred.front());
                deferred.erase(deferred.begin());
                FinishReadyResult(eastl::move(first));
            }
            // Re-queue the rest (FIFO preserved) for the next frame's Drain.
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
            for (auto it = deferred.rbegin(); it != deferred.rend(); ++it)
                m_ReadyQueue.push_front(eastl::move(*it));
        }
    }

    bool FAssetManager::HasPendingRequests() const
    {
        std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        return m_PendingCount > 0;
    }

    uint32_t FAssetManager::GetPendingRequestCount() const
    {
        std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        return m_PendingCount;
    }

    bool FAssetManager::LoadModel(const eastl::string& srcPath, FVAssetHeader& outHeader, FModelCookedData& outPayload) 
    {
        const eastl::string cookedPath = FAssetPathUtils::MakeCookedModelPath(srcPath);

#if VULTANA_WITH_COOKER
        // Cook-on-miss: missing / stale / cooker-version mismatch all end up here. IsUpToDate
        // compares source mtime+size against the Metadata section.
        if (!FVultanaCooker::IsUpToDate(srcPath, cookedPath))
        {
            VTNA_LOG_INFO("[AssetManager] cooking {} -> {}", srcPath, cookedPath);
            if (!FVultanaCooker::CookModel(srcPath, cookedPath, /*bSilent=*/true))
            {
                VTNA_LOG_ERROR("[AssetManager] cook failed for {} (dst {})", srcPath, cookedPath);
                return false;
            }
        }
#else
        struct _stat64 st = {};
        if (_stat64(cookedPath.c_str(), &st) != 0)
        {
            VTNA_LOG_ERROR("[AssetManager] no cooked asset for {} (Release build does not cook)", srcPath);
            return false;
        }
#endif

        // Read + ContentHash + cross-reference validation. VersionMismatch is expected to be
        // resolved by the cook above; Corrupt is a hard error and must be surfaced.
        const EAssetReadResult readResult = FAssetReader::ReadModelChecked(cookedPath, outHeader, outPayload);
        if (readResult != EAssetReadResult::Ok)
        {
            switch (readResult)
            {
            case EAssetReadResult::BadMagic:
                VTNA_LOG_ERROR("[AssetManager] {} is not a .vasset", cookedPath);
                break;
            case EAssetReadResult::VersionMismatch:
                VTNA_LOG_ERROR("[AssetManager] {} cooker version mismatch", cookedPath);
                break;
            case EAssetReadResult::Corrupt:
                VTNA_LOG_ERROR("[AssetManager] {} failed ContentHash/validation", cookedPath);
                break;
            default:
                VTNA_LOG_ERROR("[AssetManager] failed to read {}", cookedPath);
                break;
            }
            return false;
        }

        VTNA_LOG_INFO("[AssetManager] load {} (cooked={})", srcPath, cookedPath); // acceptance requirement
        return true;
    }

    void FAssetManager::LoadWorker(FRequest request)
    {
        FModelLoadResult result;
        result.SrcPath = request.SrcPath;

        const eastl::string cookedPath = FAssetPathUtils::MakeCookedModelPath(request.SrcPath);

#if VULTANA_WITH_COOKER
        // Cook-on-miss: missing / stale / cooker-version mismatch all end up here. IsUpToDate
        // compares source mtime+size against the Metadata section.
        if (!FVultanaCooker::IsUpToDate(request.SrcPath, cookedPath))
        {
            VTNA_LOG_INFO("[AssetManager] cooking {} -> {}", request.SrcPath, cookedPath);
            if (!FVultanaCooker::CookModel(request.SrcPath, cookedPath, /*bSilent=*/true))
            {
                VTNA_LOG_ERROR("[AssetManager] cook failed for {} (dst {})", request.SrcPath, cookedPath);
                FinishWorker(eastl::move(request), eastl::move(result));
                return;
            }
        }
#else
        struct _stat64 st = {};
        if (_stat64(cookedPath.c_str(), &st) != 0)
        {
            VTNA_LOG_ERROR("[AssetManager] no cooked asset for {} (cooker disabled by build define)", request.SrcPath);
            FinishWorker(eastl::move(request), eastl::move(result));
            return;
        }
#endif

        auto payload = eastl::make_shared<FModelCookedData>();
        const EAssetReadResult readResult = FAssetReader::ReadModelChecked(cookedPath, result.Header, *payload);
        if (readResult != EAssetReadResult::Ok)
        {
            switch (readResult)
            {
            case EAssetReadResult::BadMagic:
                VTNA_LOG_ERROR("[AssetManager] {} is not a .vasset", cookedPath);
                break;
            case EAssetReadResult::VersionMismatch:
                VTNA_LOG_ERROR("[AssetManager] {} cooker version mismatch", cookedPath);
                break;
            case EAssetReadResult::Corrupt:
                VTNA_LOG_ERROR("[AssetManager] {} failed ContentHash/validation", cookedPath);
                break;
            default:
                VTNA_LOG_ERROR("[AssetManager] failed to read {}", cookedPath);
                break;
            }
            FinishWorker(eastl::move(request), eastl::move(result));
            return;
        }

        result.bSuccess = true;
        result.Payload = eastl::move(payload);
        result.UploadBytes = EstimateUploadBytes(*result.Payload);
        VTNA_LOG_INFO("[AssetManager] load {} (cooked={})", request.SrcPath, cookedPath);

        FinishWorker(eastl::move(request), eastl::move(result));
    }

    void FAssetManager::FinishWorker(FRequest request, FModelLoadResult&& result)
    {
        eastl::vector<FModelLoadCallback> deferred;
        {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);

            // Collect deferred callbacks first so the cache refcount below can cover them.
            auto pendingIt = m_PendingCallbacks.find(request.NormalizedPath);
            if (pendingIt != m_PendingCallbacks.end())
            {
                deferred = eastl::move(pendingIt->second);
                m_PendingCallbacks.erase(pendingIt);
            }

            if (result.bSuccess)
            {
                // RefCount covers every ready-result sharing this payload: the current request
                // plus each deferred callback.
                const int32_t refs = 1 + static_cast<int32_t>(deferred.size());
                auto it = m_ModelCache.find(request.NormalizedPath);
                if (it == m_ModelCache.end())
                {
                    FCacheEntry entry;
                    entry.Payload = result.Payload;
                    entry.Header = result.Header;
                    entry.UploadBytes = result.UploadBytes;
                    entry.RefCount = refs;
                    m_ModelCache.emplace(request.NormalizedPath, eastl::move(entry));
                }
                else
                {
                    // m_Loading dedups concurrent loads, so this is defensive only.
                    it->second.RefCount += refs;
                }
            }
            m_Loading.erase(request.NormalizedPath);
        }

        // Share the payload with the deferred requests (refcount above covers them).
        if (result.bSuccess)
        {
            for (auto& cb : deferred)
            {
                FReadyResult ready;
                ready.NormalizedPath = request.NormalizedPath;
                ready.Callback = eastl::move(cb);
                ready.Result = result; // shared_ptr copy is cheap
                PushReadyResult(eastl::move(ready));
            }
        }
        else
        {
            for (auto& cb : deferred)
            {
                FReadyResult ready;
                ready.NormalizedPath = request.NormalizedPath;
                ready.Callback = eastl::move(cb);
                ready.Result.bSuccess = false;
                ready.Result.SrcPath = request.SrcPath;
                PushReadyResult(eastl::move(ready));
            }
        }

        PushReadyResult({request.NormalizedPath, eastl::move(request.Callback), eastl::move(result)});
    }

    void FAssetManager::SpawnWorker(FRequest request)
    {
        auto* pTask = new FAsyncModelLoadTask(this, eastl::move(request));
        {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
            m_Tasks.push_back(pTask);
        }
        m_pTaskScheduler->AddTaskSetToPipe(pTask);
    }

    void FAssetManager::PushReadyResult(FReadyResult&& result)
    {
        std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        m_ReadyQueue.push_back(eastl::move(result));
    }

    void FAssetManager::FinishReadyResult(FReadyResult&& result)
    {
        // Fire the callback on the main thread — this is where GPU resources get created
        // (ResourceCache::GetSceneBuffer -> AllocateSceneStaticBuffer -> staging upload).
        if (result.Callback)
        {
            result.Callback(eastl::move(result.Result));
        }

        // Release the cache refcount held by this result; evict at zero. A consumer that kept
        // its own shared_ptr copy keeps the payload alive past the eviction.
        std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        auto it = m_ModelCache.find(result.NormalizedPath);
        if (it != m_ModelCache.end())
        {
            it->second.RefCount--;
            if (it->second.RefCount <= 0)
            {
                m_ModelCache.erase(it);
            }
        }
        if (m_PendingCount > 0)
        {
            --m_PendingCount;
        }
    }

    uint32_t FAssetManager::EstimateUploadBytes(const FModelCookedData& payload)
    {
        uint64_t total = 0;
        for (const FMeshCookedData& mesh : payload.Meshes)
        {
            total += mesh.IndexData.size();
            for (const FMeshCookedData::FStream& stream : mesh.Streams)
                total += stream.Data.size();
            total += mesh.MeshletBounds.size() * sizeof(FMeshlet);
            total += mesh.MeshletIndexData.size();
            total += mesh.MeshletVertexData.size();
        }
        return (uint32_t)eastl::min<uint64_t>(total, UINT32_MAX);
    }
} // namespace Assets
