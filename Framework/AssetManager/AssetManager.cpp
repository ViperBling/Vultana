#include "AssetManager/AssetManager.hpp"

#include "AssetCooker/VultanaCooker.hpp"
#include "AssetCooker/AssetPathUtils.hpp"
#include "AssetCooker/AssetValidate.hpp"

#include "Utilities/Log.hpp"

#include "Core/VultanaEngine.hpp"

#include <sys/stat.h>
#include "AssetManager.hpp"

namespace Assets
{
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
}
