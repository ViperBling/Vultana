#pragma once

#include "AssetCooker/VAssetTypes.hpp"
#include "AssetCooker/BinaryIO.hpp"

namespace Assets
{
    enum class EAssetReadResult : uint32_t
    {
        Ok              = 0,
        NotFound        = 1,        // file missing / unreadable
        BadMagic        = 2,        // not a .vasset
        VersionMismatch = 3,        // old cooker version -> re-cook
        Corrupt         = 4,        // hash mismatch / parse fail / ValidateModel fail
    };

    class FAssetReader
    {
    public:
        // Read header only (fixed header + Metadata section). Does not parse the payload.
        static bool ReadHeader(const eastl::string& path, FVAssetHeader& outHeader);
        // Full read: header + section locate + ContentHash check + ValidateModel. Reason code.
        static EAssetReadResult ReadModelChecked(const eastl::string& path, FVAssetHeader& outHeader, FModelCookedData& outPayload);
        // Thin wrapper over ReadModelChecked for existing call sites (tests, Cooker).
        static bool ReadModel(const eastl::string& path, FVAssetHeader& outHeader, FModelCookedData& outPayload);
        static EAssetReadResult ReadTextureChecked(const eastl::string& path, FVAssetHeader& outHeader, FTextureCookedData& outPayload);
        static bool ReadTexture(const eastl::string& path, FVAssetHeader& outHeader, FTextureCookedData& outPayload);

    private:
        static bool ReadHeaderRegion(FBinaryReader& reader, FVAssetHeader& outHeader);
        static bool ReadModelPayload(FBinaryReader& reader, FModelCookedData& outPayload);
    };
}