#include "MeshMaterial.hpp"

namespace Scene
{
    MeshMaterial::~MeshMaterial()
    {
        
    }

    RHI::RHIPipelineState *MeshMaterial::GetPSO()
    {
        return nullptr;
    }

    void MeshMaterial::UpdateConstants()
    {
        mMaterialCB.ShadingModel = (uint)mShadingModel;
        mMaterialCB.Albedo = mAlbedoColor;
        mMaterialCB.Emissive = mEmissiveColor;
        mMaterialCB.Metallic = mMetallic;
        mMaterialCB.Roughness = mRoughness;
        mMaterialCB.AlphaCutout = mAlphaCutout;
        mMaterialCB.Diffuse = mDiffuseColor;
        mMaterialCB.Specular = mSpecularColor;
        mMaterialCB.Glossiness = mGlossiness;

        mMaterialCB.bPBRMetallicRoughness = mWorkFlow == MaterialWorkFlow::PBRMetallicRoughness;
        mMaterialCB.bPBRSpecularGlossiness = mWorkFlow == MaterialWorkFlow::PBRSpecularGlossiness;
        mMaterialCB.bRGNormalTexture = mpNormalTexture && (mpNormalTexture->GetTexture()->GetDesc().Format == RHI::ERHIFormat::BC5UNORM);
        mMaterialCB.bDoubleSided = mbDoubleSided;
    }

    void MeshMaterial::OnGUI()
    {
    }

    void MeshMaterial::AddMaterialDefines(std::vector<std::string> &defines)
    {
    }
}