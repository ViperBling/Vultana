#pragma once

#include "AssetCooker/FVAssetTypes.hpp"
#include "AssetCooker/FBinaryIO.hpp"

namespace Assets
{
    class FAssetReader
    {
    public:
        // Read header only (fixed header + Metadata section: source path + reference table). Does not parse the payload.
        static bool ReadHeader(const eastl::string& path, FVAssetHeader& outHeader);
        // Reads the full Model payload. Validates magic + version + type first.
        static bool ReadModel(const eastl::string& path, FVAssetHeader& outHeader, FModelCookedData& outPayload);
        // Reads the full ETexture payload (used by Slice 5).
        static bool ReadTexture(const eastl::string& path, FVAssetHeader& outHeader, FTextureCookedData& outPayload);

    private:
        static bool ReadHeaderRegion(FBinaryReader& reader, FVAssetHeader& outHeader);
        static bool ReadModelPayload(FBinaryReader& reader, FModelCookedData& outPayload);
    };
}