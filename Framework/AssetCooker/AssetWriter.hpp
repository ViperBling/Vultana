#pragma once

#include "AssetCooker/VAssetTypes.hpp"
#include "AssetCooker/BinaryIO.hpp"

namespace Assets
{
    class FAssetWriter
    {
    public:
        // Writes a Model .vasset to disk. Fixed header + section table + Metadata + Model payload are all written here.
        static bool Write(const eastl::string& dstPath, const FVAssetHeader& header, const FModelCookedData& payload);
        // Writes a Texture .vasset to disk.
        static bool Write(const eastl::string& dstPath, const FVAssetHeader& header, const FTextureCookedData& payload);
        static bool IsUpToDate(const eastl::string& srcPath, const eastl::string& cookedPath);

    private:
        static void WriteHeader(FBinaryWriter& writer, const FVAssetHeader& header);
        static void WriteSectionDesc(FBinaryWriter& writer, const FAssetSectionDesc& desc);
        static void WriteMetadataSection(FBinaryWriter& writer, const FVAssetHeader& header);
        static void WriteModelPayload(FBinaryWriter& writer, const FModelCookedData& payload);
    };
}