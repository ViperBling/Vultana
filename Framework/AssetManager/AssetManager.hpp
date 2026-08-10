#pragma once

#include "AssetCooker/VAssetTypes.hpp"
#include "AssetCooker/AssetReader.hpp"

#include <EASTL/deque.h>
#include <EASTL/functional.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include <mutex>
#include <cstdint>

namespace Scene
{
    class FWorld;
}

namespace enki
{
    class TaskScheduler;
}

namespace Assets
{
    // Async model load result. The payload is a shared_ptr so the FAssetManager cache and its
    // consumers can coexist: a consumer that keeps a copy outlives a cache eviction, and a
    // consumer that drops it lets the CPU payload be freed once the cache is gone.
    struct FModelLoadResult
    {
        bool bSuccess = false;
        eastl::string SrcPath;                          // Original request path (logging)
        FVAssetHeader Header;
        eastl::shared_ptr<FModelCookedData> Payload;
        uint32_t UploadBytes = 0;
    };

    class FAssetManager
    {
    public:
        explicit FAssetManager(Scene::FWorld* inWorld, enki::TaskScheduler* pTaskScheduler) : m_pWorld(inWorld), m_pTaskScheduler(pTaskScheduler) {}
        ~FAssetManager();

        using FModelLoadCallback = eastl::function<void(FModelLoadResult&&)>;

        void RequestModel(const eastl::string& srcPath, FModelLoadCallback&& callback);
        void DrainReadyQueue();

        bool HasPendingRequests() const;
        uint32_t GetPendingRequestCount() const;

        bool LoadModel(const eastl::string& srcPath, FVAssetHeader& outHeader, FModelCookedData& outPayload);

    private:
        struct FRequest
        {
            eastl::string SrcPath;
            eastl::string NormalizedPath;
            FModelLoadCallback Callback;
        };

        struct FReadyResult
        {
            eastl::string NormalizedPath;
            FModelLoadCallback Callback;
            FModelLoadResult Result;
        };

        struct FCacheEntry
        {
            eastl::shared_ptr<FModelCookedData> Payload;
            FVAssetHeader Header;
            uint32_t UploadBytes = 0;
            int32_t RefCount = 0;
        };

        void LoadWorker(FRequest request);                                              // Enki worker thread
        void FinishWorker(FRequest request, FModelLoadResult&& result);                 // Worker thread tail
        void SpawnWorker(FRequest request);                                             // Main thread
        void PushReadyResult(FReadyResult&& result);                                    // Any thread (locked)
        void FinishReadyResult(FReadyResult&& result);                                  // Main thread: fire callback + refcount
        static uint32_t EstimateUploadBytes(const FModelCookedData& payload);

    private:
        class FAsyncModelLoadTask;

        Scene::FWorld* m_pWorld = nullptr;
        enki::TaskScheduler* m_pTaskScheduler = nullptr;
        mutable std::recursive_mutex m_Mutex;                                                 // Mutable: const accessors (HasPendingLoads/GetPendingCount) lock it
        eastl::deque<FReadyResult> m_ReadyQueue;                                              // Worker -> main ("ready" queue)
        eastl::hash_map<eastl::string, FCacheEntry> m_ModelCache;                             // Normalized -> entry
        eastl::hash_map<eastl::string, eastl::vector<FModelLoadCallback>> m_PendingCallbacks; // Normalized -> deferred
        eastl::hash_set<eastl::string> m_Loading;                                             // Normalized paths with a worker in flight
        eastl::vector<FAsyncModelLoadTask*> m_Tasks;                                          // Fire-and-forget enki tasks (retained until shutdown)
        uint32_t m_PendingCount = 0;                                                          // Requests not yet delivered (guarded by m_Mutex)
        static constexpr uint32_t kMaxUploadBytesPerFrame = 32u * 1024u * 1024u;
    };
}