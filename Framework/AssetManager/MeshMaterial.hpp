#pragma once

#include "Renderer/RendererBase.hpp"
#include "Common/ModelConstants.hlsli"
#include "Common/ShadingModel.hlsli"

namespace Scene
{
    class FWorld;
}

namespace Assets
{
    enum class MaterialWorkFlow
    {
        PBRMetallicRoughness,
        PBRSpecularGlossiness
    };

    class FMeshMaterial
    {
        friend class Scene::FWorld;
        friend class FModelLoader;

    public:
        ~FMeshMaterial();

        RHI::FRHIPipelineState* GetPSO();
        RHI::FRHIPipelineState* GetIDPSO();
        RHI::FRHIPipelineState* GetOutlinePSO();

        RHI::FRHIPipelineState* GetMeshletPSO();

        RHI::FRHIPipelineState* GetVertexSkinningPSO();

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

        RHI::FRHIPipelineState* m_pPSO = nullptr;
        RHI::FRHIPipelineState* m_pIDPSO = nullptr;
        RHI::FRHIPipelineState* m_pOutlinePSO = nullptr;

        RHI::FRHIPipelineState* m_pMeshletPSO = nullptr;

        RHI::FRHIPipelineState* m_pVertexSkinningPSO = nullptr;

        EShadingModel m_ShadingModel = EShadingModel::DefaultPBR;

        RenderResources::FTexture2D* m_pDiffuseTexture = nullptr;
        RenderResources::FTexture2D* m_pSpecularGlossinessTexture = nullptr;
        float3 m_DiffuseColor = float3(1.0f, 1.0f, 1.0f);
        float3 m_SpecularColor = float3(0.0f, 0.0f, 0.0f);
        float m_Glossiness = 0.0f;

        RenderResources::FTexture2D* m_pAlbedoTexture = nullptr;
        RenderResources::FTexture2D* m_pMetallicRoughTexture = nullptr;
        float3 m_AlbedoColor = float3(1.0f, 1.0f, 1.0f);
        float m_Metallic = 0.0f;
        float m_Roughness = 0.0f;

        RenderResources::FTexture2D* m_pNormalTexture = nullptr;
        RenderResources::FTexture2D* m_pEmissiveTexture = nullptr;
        RenderResources::FTexture2D* m_pAOTexture = nullptr;
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