#pragma once

#include "AssetCooker/VAssetTypes.hpp"
#include "AssetCooker/Importer.hpp"

namespace Assets
{
    // Bakes a neutral FImportPrimitive into disk-facing FMeshCookedData:
    // meshopt remap + (static only) meshlet build + bounding sphere.
    class FMeshCooker
    {
    public:
        static bool Cook(const FImportPrimitive& in, FMeshCookedData& out);

    private:
        static bool CookSkeletal(const FImportPrimitive& in, FMeshCookedData& out);
        static bool CookStatic(const FImportPrimitive& in, FMeshCookedData& out);
    };
}