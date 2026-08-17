#pragma once
#include "AssetCooker/VAssetTypes.hpp"
#include "Utilities/Math.hpp"
#include "RHI/RHICommon.hpp"
#include "Common/ModelConstants.hlsli"

namespace RenderResources { class FTexture2D; }

namespace Assets
{
    class FMeshMaterial;

    class FModelLoader final
    {
    public:
        static FMeshMaterial* BuildMaterial(const FAssetMaterial* assetMat, bool bSkeletalAnim, bool bFrontFaceCCW);
        static FMaterialTextureInfo BuildTextureInfo(const RenderResources::FTexture2D* texture, const FAssetTextureRef& ref);
    };
}
