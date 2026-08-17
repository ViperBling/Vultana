#include "AssetManager/ModelLoader.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "AssetManager/AssetFormatMapping.hpp"

#include "AssetCooker/AssetPathUtils.hpp"

#include "ResourceCache.hpp"
#include "Core/VultanaEngine.hpp"

#include "Utilities/Math.hpp"

#include <EASTL/string.h>

namespace Assets
{
    FMeshMaterial* FModelLoader::BuildMaterial(const FAssetMaterial* assetMat, bool bSkeletalAnim, bool bFrontFaceCCW)
    {
        FMeshMaterial* material = new FMeshMaterial;
        material->m_bSkeletalAnim = bSkeletalAnim;
        material->m_bFrontFaceCCW = bFrontFaceCCW;
        if (assetMat == nullptr)
        {
            return material; // untextured primitive: engine defaults
        }
        material->m_Name = assetMat->Name;
        material->m_WorkFlow = ToEngineWorkFlow(assetMat->WorkFlow);
        material->m_AlbedoColor = assetMat->AlbedoColor;
        material->m_Metallic = assetMat->Metallic;
        material->m_Roughness = assetMat->Roughness;
        material->m_DiffuseColor = assetMat->DiffuseColor;
        material->m_SpecularColor = assetMat->SpecularColor;
        material->m_Glossiness = assetMat->Glossiness;
        material->m_EmissiveColor = assetMat->EmissiveColor;
        material->m_AlphaCutout = assetMat->AlphaCutout;
        material->m_bAlphaTest = assetMat->bAlphaTest;
        material->m_bAlphaBlend = assetMat->bAlphaBlend;
        material->m_bDoubleSided = assetMat->bDoubleSided;

        auto* textureCache = FResourceCache::GetInstance();
        auto resolveTexture = [&](const FAssetTextureRef& ref) -> RenderResources::FTexture2D*
        {
            const eastl::string srcRel = FAssetPathUtils::GetTextureSourcePath(ref.RelativePath);
            if (FAssetPathUtils::IsDDSSource(srcRel))
            {
                return textureCache->GetTexture2D(Core::FVultanaEngine::GetEngineInstance()->GetAssetsPath() + srcRel, ref.bSRGB);
            }
            return textureCache->GetTexture2DProductReady(FAssetPathUtils::GetContentRootPath() + ref.RelativePath);
        };

        for (const FAssetTextureRef& ref : assetMat->Textures)
        {
            switch (ref.Usage)
            {
            case ETextureUsage::BaseColor:
                material->m_pAlbedoTexture = resolveTexture(ref);
                material->m_MaterialCB.AlbedoTexture = BuildTextureInfo(material->m_pAlbedoTexture, ref);
                break;
            case ETextureUsage::Normal:
                material->m_pNormalTexture = resolveTexture(ref);
                material->m_MaterialCB.NormalTexture = BuildTextureInfo(material->m_pNormalTexture, ref);
                break;
            case ETextureUsage::Emissive:
                material->m_pEmissiveTexture = resolveTexture(ref);
                material->m_MaterialCB.EmissiveTexture = BuildTextureInfo(material->m_pEmissiveTexture, ref);
                break;
            case ETextureUsage::AmbientOcclusion:
                material->m_pAOTexture = resolveTexture(ref);
                material->m_MaterialCB.AmbientOcclusionTexture = BuildTextureInfo(material->m_pAOTexture, ref);
                break;
            case ETextureUsage::MetallicRoughness:
                material->m_pMetallicRoughTexture = resolveTexture(ref);
                material->m_MaterialCB.MetallicRoughnessTexture = BuildTextureInfo(material->m_pMetallicRoughTexture, ref);
                break;
            case ETextureUsage::Diffuse:
                material->m_pDiffuseTexture = resolveTexture(ref);
                material->m_MaterialCB.DiffuseTexture = BuildTextureInfo(material->m_pDiffuseTexture, ref);
                break;
            case ETextureUsage::SpecularGlossiness:
                material->m_pSpecularGlossinessTexture = resolveTexture(ref);
                material->m_MaterialCB.SpecularGlossinessTexture = BuildTextureInfo(material->m_pSpecularGlossinessTexture, ref);
                break;
            default:
                break;
            }
        }
        return material;
    }

    FMaterialTextureInfo FModelLoader::BuildTextureInfo(const RenderResources::FTexture2D* texture, const FAssetTextureRef& ref)
    {
        FMaterialTextureInfo info;
        if (texture)
        {
            info.Index = texture->GetSRV()->GetHeapIndex();
            info.Width = texture->GetTexture()->GetDesc().Width;
            info.Height = texture->GetTexture()->GetDesc().Height;
            if (ref.bHasTransform)
            {
                info.IsTransform = true;
                info.Offset = ref.Offset;
                info.Scale = ref.Scale;
                info.Rotation = ref.Rotation;
            }
        }
        return info;
    }
} // namespace Assets
