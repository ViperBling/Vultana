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

    struct FTextureLoadResult
    {
        bool bSuccess = false;
        eastl::string SrcPath;                          // source path (logging)
        eastl::string CookedPath;                       // product path (cache key / logging)
        uint64_t ContentHash = 0;                       // product content hash (cache key / logging)
        eastl::shared_ptr<FTextureCookedData> Payload;
    };

    class FAssetManager
    {
    public:
        explicit FAssetManager(Scene::FWorld* inWorld, enki::TaskScheduler* pTaskScheduler) : m_pWorld(inWorld), m_pTaskScheduler(pTaskScheduler) {}
        ~FAssetManager();

        using FModelLoadCallback = eastl::function<void(FModelLoadResult&&)>;
        using FTextureLoadCallback = eastl::function<void(FTextureLoadResult&&)>;

        void RequestTexture(const eastl::string& productPath, FTextureLoadCallback&& callback);
        void RequestTextureReload(const eastl::string& productPath, FTextureLoadCallback&& callback);
        bool IsTextureProductStale(const eastl::string& productPath) const;

        void RequestModelTextures(const FModelCookedData& model, eastl::function<void()> onAllReady);
        void RequestModel(const eastl::string& srcPath, FModelLoadCallback&& callback);
        void DrainReadyQueue();

        bool HasPendingRequests() const;
        uint32_t GetPendingRequestCount() const;

        bool LoadModel(const eastl::string& srcPath, FVAssetHeader& outHeader, FModelCookedData& outPayload);
        void RequestModelReload(const eastl::string& srcPath, FModelLoadCallback&& callback);

    private:
        struct FModelRequest
        {
            eastl::string SrcPath;
            eastl::string NormalizedPath;
            FModelLoadCallback Callback;
            bool bForceCook = false;
        };

        struct FReadyModelRequest
        {
            eastl::string NormalizedPath;
            FModelLoadCallback Callback;
            FModelLoadResult Result;
        };

        struct FModelCacheEntry
        {
            eastl::shared_ptr<FModelCookedData> Payload;
            FVAssetHeader Header;
            uint32_t UploadBytes = 0;
            int32_t RefCount = 0;
        };

        struct FTextureRequest
        {
            eastl::string ProductPath;
            eastl::string NormalizedPath;
            FTextureLoadCallback Callback;
            bool bForceCook = false;
        };

        struct FReadyTextureResult
        {
            eastl::string NormalizedPath;
            FTextureLoadCallback Callback;
            FTextureLoadResult Result;
        };

        struct FTextureCacheEntry
        {
            eastl::shared_ptr<FTextureCookedData> Payload;
            uint64_t ContentHash = 0;
            int32_t RefCount = 0;
        };

        void ModelLoadWorker(FModelRequest request);                                            // Enki worker thread
        void FinishModelLoadWorker(FModelRequest request, FModelLoadResult&& result);           // Worker thread tail
        void SpawnModelLoadWorker(FModelRequest request);                                       // Main thread or worker tail
        void PushReadyModelResult(FReadyModelRequest&& result);                                 // Any thread (locked)
        void FinishReadyModelResult(FReadyModelRequest&& result);                               // Main thread: fire callback + refcount
        static uint32_t EstimateUploadBytes(const FModelCookedData& payload);

        void TextureLoadWorker(FTextureRequest request);                                         // Enki worker thread
        void FinishTextureLoadWorker(FTextureRequest request, FTextureLoadResult&& result);      // Worker thread tail
        void SpawnTextureLoadWorker(FTextureRequest request);                                    // Main thread or worker tail
        void PushReadyTextureResult(FReadyTextureResult&& result);                               // Any thread (locked)
        void FinishReadyTextureResult(FReadyTextureResult&& result);                             // Main thread: GPU create + callback + refcount

    private:
        class FAsyncModelLoadTask;
        class FAsyncTextureLoadTask;

        Scene::FWorld* m_pWorld = nullptr;
        enki::TaskScheduler* m_pTaskScheduler = nullptr;
        mutable std::recursive_mutex m_Mutex;                                                           // Mutable: const accessors (HasPendingLoads/GetPendingCount) lock it
        eastl::deque<FReadyModelRequest> m_ReadyQueue;                                                  // Worker -> main ("ready" queue)
        eastl::hash_map<eastl::string, FModelCacheEntry> m_ModelCache;                                  // Normalized -> entry
        eastl::hash_map<eastl::string, eastl::vector<FModelLoadCallback>> m_ModelPendingCallbacks;      // Normalized -> deferred
        eastl::hash_map<eastl::string, eastl::vector<FModelLoadCallback>> m_ModelReloadCallbacks;       // Normalized -> deferred
        eastl::hash_set<eastl::string> m_Loading;                                                       // Normalized paths with a worker in flight
        eastl::vector<FAsyncModelLoadTask*> m_ModelTasks;                                               // Fire-and-forget enki tasks (retained until shutdown)

        eastl::deque<FReadyTextureResult> m_TextureReadyQueue;                                          // Worker -> main
        eastl::hash_map<eastl::string, FTextureCacheEntry> m_TextureCache;                              // Normalized product path -> entry
        eastl::hash_map<eastl::string, eastl::vector<FTextureLoadCallback>> m_TexturePendingCallbacks;  // Normalized -> deferred
        eastl::hash_map<eastl::string, eastl::vector<FTextureLoadCallback>> m_TextureReloadCallbacks;   
        eastl::hash_set<eastl::string> m_TextureLoading;                                                // Normalized paths with a worker in flight
        eastl::vector<FAsyncTextureLoadTask*> m_TextureTasks;       

        uint32_t m_PendingCount = 0;                                                                    // Requests not yet delivered (guarded by m_Mutex)
        static constexpr uint32_t kMaxUploadBytesPerFrame = 32u * 1024u * 1024u;
    };
}