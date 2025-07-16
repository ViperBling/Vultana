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

        RHI::RHIPipelineState* GetMeshletPSO();

        RHI::RHIPipelineState* GetVertexSkinningPSO();

        void UpdateConstants();
        const FModelMaterialConstants* GetMaterialConstants() const { return &m_MaterialCB; }
        void OnGUI();

        bool IsFrontFaceCCW() const { return m_bFrontFaceCCW; }
        bool IsDoubleSided() const { return m_bDoubleSided; }
        bool IsAlphaBlend() const { return m_bAlphaBlend; }
        bool IsAlphaTest() const { return m_bAlphaTest; }
        bool IsVertexSkinned() const { return m_bSkeletalAnim; }

    private:
        void AddMaterialDefines(eastl::vector<eastl::string>& defines);

    private:
        eastl::string m_Name;
        FModelMaterialConstants m_MaterialCB = {};

        RHI::RHIPipelineState* m_pPSO = nullptr;
        RHI::RHIPipelineState* m_pIDPSO = nullptr;
        RHI::RHIPipelineState* m_pOutlinePSO = nullptr;

        RHI::RHIPipelineState* m_pMeshletPSO = nullptr;

        RHI::RHIPipelineState* m_pVertexSkinningPSO = nullptr;

        EShadingModel m_ShadingModel = EShadingModel::DefaultPBR;

        RenderResources::Texture2D* m_pDiffuseTexture = nullptr;
        RenderResources::Texture2D* m_pSpecularGlossinessTexture = nullptr;
        float3 m_DiffuseColor = float3(1.0f, 1.0f, 1.0f);
        float3 m_SpecularColor = float3(0.0f, 0.0f, 0.0f);
        float m_Glossiness = 0.0f;

        RenderResources::Texture2D* m_pAlbedoTexture = nullptr;
        RenderResources::Texture2D* m_pMetallicRoughTexture = nullptr;
        float3 m_AlbedoColor = float3(1.0f, 1.0f, 1.0f);
        float m_Metallic = 0.0f;
        float m_Roughness = 0.0f;

        RenderResources::Texture2D* m_pNormalTexture = nullptr;
        RenderResources::Texture2D* m_pEmissiveTexture = nullptr;
        RenderResources::Texture2D* m_pAOTexture = nullptr;
        float3 m_EmissiveColor = float3(0.0f, 0.0f, 0.0f);
        float m_AlphaCutout = 0.0f;
        float m_bAlphaTest = false;

        bool m_bAlphaBlend = false;
        bool m_bSkeletalAnim = false;
        bool m_bFrontFaceCCW = false;
        bool m_bDoubleSided = false;
        bool m_bPBRSpecularGlossiness = false;

        MaterialWorkFlow m_WorkFlow = MaterialWorkFlow::PBRMetallicRoughness;
    };
}