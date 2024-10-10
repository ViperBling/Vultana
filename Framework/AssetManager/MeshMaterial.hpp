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

    public:
        ~MeshMaterial();

        RHI::RHIPipelineState* GetPSO();

        void UpdateConstants();
        const FModelMaterialConstants& GetMaterialConstants() const { return mMaterialCB; }
        void OnGUI();

        bool IsFrontFaceCCW() const { return mbFrontFaceCCW; }
        bool IsDoubleSided() const { return mbDoubleSided; }
        bool IsAlphaBlend() const { return mbAlphaBlend; }
        bool IsAlphaTest() const { return mbAlphaTest; }

    private:
        void AddMaterialDefines(std::vector<std::string>& defines);

    private:
        std::string mName;
        FModelMaterialConstants mMaterialCB = {};

        RHI::RHIPipelineState* mpPSO = nullptr;

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
        bool mbFrontFaceCCW = false;
        bool mbDoubleSided = false;
        bool mbPBRSpecularGlossiness = false;

        MaterialWorkFlow mWorkFlow = MaterialWorkFlow::PBRMetallicRoughness;
    };
}