#pragma once

#include "Renderer/RendererBase.hpp"
#include "Common/ModelConstants.hlsli"
#include "Common/ShadingModel.hlsli"

namespace Scene
{
    class World;
}

namespace Assets
{
    enum class MaterialWorkFlow
    {
        PBRMetallicRoughness,
        PBRSpecularGlossiness
    };

    class MeshMaterial
    {
        friend class Scene::World;
        friend class ModelLoader;

    public:
        ~MeshMaterial();

        RHI::RHIPipelineState* GetPSO();
        RHI::RHIPipelineState* GetIDPSO();
        RHI::RHIPipelineState* GetOutlinePSO();

        RHI::RHIPipelineState* GetVertexSkinningPSO();

        void UpdateConstants();
        const FModelMaterialConstants* GetMaterialConstants() const { return &mMaterialCB; }
        void OnGUI();

        bool IsFrontFaceCCW() const { return mbFrontFaceCCW; }
        bool IsDoubleSided() const { return mbDoubleSided; }
        bool IsAlphaBlend() const { return mbAlphaBlend; }
        bool IsAlphaTest() const { return mbAlphaTest; }
        bool IsVertexSkinned() const { return mbSkeletalAnim; }

    private:
        void AddMaterialDefines(eastl::vector<eastl::string>& defines);

    private:
        eastl::string mName;
        FModelMaterialConstants mMaterialCB = {};

        RHI::RHIPipelineState* mpPSO = nullptr;
        RHI::RHIPipelineState* mpIDPSO = nullptr;
        RHI::RHIPipelineState* mpOutlinePSO = nullptr;

        RHI::RHIPipelineState* mpVertexSkinningPSO = nullptr;

        EShadingModel mShadingModel = EShadingModel::DefaultPBR;

        RenderResources::Texture2D* mpDiffuseTexture = nullptr;
        RenderResources::Texture2D* mpSpecularGlossinessTexture = nullptr;
        float3 mDiffuseColor = float3(1.0f, 1.0f, 1.0f);
        float3 mSpecularColor = float3(0.0f, 0.0f, 0.0f);
        float mGlossiness = 0.0f;

        RenderResources::Texture2D* mpAlbedoTexture = nullptr;
        RenderResources::Texture2D* mpMetallicRoughTexture = nullptr;
        float3 mAlbedoColor = float3(1.0f, 1.0f, 1.0f);
        float mMetallic = 0.0f;
        float mRoughness = 0.0f;

        RenderResources::Texture2D* mpNormalTexture = nullptr;
        RenderResources::Texture2D* mpEmissiveTexture = nullptr;
        RenderResources::Texture2D* mpAOTexture = nullptr;
        float3 mEmissiveColor = float3(0.0f, 0.0f, 0.0f);
        float mAlphaCutout = 0.0f;
        float mbAlphaTest = false;

        bool mbAlphaBlend = false;
        bool mbSkeletalAnim = false;
        bool mbFrontFaceCCW = false;
        bool mbDoubleSided = false;
        bool mbPBRSpecularGlossiness = false;

        MaterialWorkFlow mWorkFlow = MaterialWorkFlow::PBRMetallicRoughness;
    };
}