#include "MeshMaterial.hpp"
#include "ResourceCache.hpp"
#include "Core/VultanaEngine.hpp"

namespace Assets
{
    MeshMaterial::~MeshMaterial()
    {
        auto resourceCache = ResourceCache::GetInstance();
        resourceCache->ReleaseTexture2D(m_pDiffuseTexture);
        resourceCache->ReleaseTexture2D(m_pSpecularGlossinessTexture);
        resourceCache->ReleaseTexture2D(m_pAlbedoTexture);
        resourceCache->ReleaseTexture2D(m_pMetallicRoughTexture);
        resourceCache->ReleaseTexture2D(m_pNormalTexture);
        resourceCache->ReleaseTexture2D(m_pEmissiveTexture);
        resourceCache->ReleaseTexture2D(m_pAOTexture);
    }

    RHI::RHIPipelineState *MeshMaterial::GetPSO()
    {
        if (m_pPSO == nullptr)
        {
            Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

            eastl::vector<eastl::string> defines;
            AddMaterialDefines(defines);
            defines.push_back("UNIFORM_RESOURCE=1");

            if (m_pAlbedoTexture) defines.push_back("ALBEDO_TEXTURE=1");
            if (m_pDiffuseTexture) defines.push_back("DIFFUSE_TEXTURE=1");
            if (m_bAlphaTest) defines.push_back("ALPHA_TEST=1");

            RHI::RHIGraphicsPipelineStateDesc psoDesc {};
            psoDesc.VS = pRenderer->GetShader("Model.hlsl", "VSMain", RHI::ERHIShaderType::VS, defines);
            psoDesc.PS = pRenderer->GetShader("Model.hlsl", "PSMain", RHI::ERHIShaderType::PS, defines);
            psoDesc.RasterizerState.CullMode = m_bDoubleSided ? RHI::ERHICullMode::None : RHI::ERHICullMode::Back;
            psoDesc.RasterizerState.bFrontCCW = m_bFrontFaceCCW;
            psoDesc.DepthStencilState.bDepthTest = true;
            psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::GreaterEqual;
            // For now, we only implement forward rendering, so we only need one RT format
            psoDesc.RTFormats[0] = RHI::ERHIFormat::RGBA8SRGB;          // Diffuse RT
            // psoDesc.RTFormats[1] = RHI::ERHIFormat::RGBA8SRGB;          // Specular RT
            // psoDesc.RTFormats[2] = RHI::ERHIFormat::RGBA8UNORM;         // Normal RT
            // psoDesc.RTFormats[3] = RHI::ERHIFormat::R11G11B10F;         // Emissive RT
            // psoDesc.RTFormats[4] = RHI::ERHIFormat::RGBA32F;            // CustomDataRT
            psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

            m_pPSO = pRenderer->GetPipelineState(psoDesc, m_Name + "_ModelPSO");
        }
        return m_pPSO;
    }

    RHI::RHIPipelineState *MeshMaterial::GetIDPSO()
    {
        if (m_pIDPSO == nullptr)
        {
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

            eastl::vector<eastl::string> defines;
            defines.push_back("UNIFORM_RESOURCE=1");

            if (m_pAlbedoTexture) defines.push_back("ALBEDO_TEXTURE=1");
            if (m_pDiffuseTexture) defines.push_back("DIFFUSE_TEXTURE=1");
            if (m_bAlphaTest) defines.push_back("ALPHA_TEST=1");

            RHI::RHIGraphicsPipelineStateDesc psoDesc {};
            psoDesc.VS = pRenderer->GetShader("ModelID.hlsl", "VSMain", RHI::ERHIShaderType::VS, defines);
            psoDesc.PS = pRenderer->GetShader("ModelID.hlsl", "PSMain", RHI::ERHIShaderType::PS, defines);
            psoDesc.RasterizerState.CullMode = m_bDoubleSided ? RHI::ERHICullMode::None : RHI::ERHICullMode::Back;
            psoDesc.RasterizerState.bFrontCCW = m_bFrontFaceCCW;
            psoDesc.DepthStencilState.bDepthTest = true;
            psoDesc.DepthStencilState.bDepthWrite = false;
            psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::GreaterEqual;
            psoDesc.RTFormats[0] = RHI::ERHIFormat::R32UI;
            psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

            m_pIDPSO = pRenderer->GetPipelineState(psoDesc, m_Name + "_ModelIDPSO");
        }
        return m_pIDPSO;
    }

    RHI::RHIPipelineState *MeshMaterial::GetOutlinePSO()
    {
        if (m_pOutlinePSO == nullptr)
        {
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

            eastl::vector<eastl::string> defines;
            defines.push_back("UNIFORM_RESOURCE=1");
            
            if (m_bAlphaTest) defines.push_back("ALPHA_TEST=1");

            RHI::RHIGraphicsPipelineStateDesc psoDesc {};
            psoDesc.VS = pRenderer->GetShader("ModelOutline.hlsl", "VSMain", RHI::ERHIShaderType::VS, defines);
            psoDesc.PS = pRenderer->GetShader("ModelOutline.hlsl", "PSMain", RHI::ERHIShaderType::PS, defines);
            psoDesc.RasterizerState.CullMode = RHI::ERHICullMode::Front;
            psoDesc.RasterizerState.bFrontCCW = m_bFrontFaceCCW;
            psoDesc.DepthStencilState.bDepthTest = true;
            psoDesc.DepthStencilState.bDepthWrite = false;
            psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::GreaterEqual;
            psoDesc.RTFormats[0] = RHI::ERHIFormat::RGBA8SRGB;
            psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

            m_pOutlinePSO = pRenderer->GetPipelineState(psoDesc, m_Name + "_ModelOutlinePSO");
        }
        return m_pOutlinePSO;
    }

    RHI::RHIPipelineState *MeshMaterial::GetMeshletPSO()
    {
        if (m_pMeshletPSO == nullptr)
        {
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

            eastl::vector<eastl::string> defines;
            AddMaterialDefines(defines);

            RHI::RHIMeshShadingPipelineStateDesc psoDesc {};
            psoDesc.AS = pRenderer->GetShader("MeshletCulling.hlsl", "ASMain", RHI::ERHIShaderType::AS, defines);
            psoDesc.MS = pRenderer->GetShader("ModelMeshlet.hlsl", "MSMain", RHI::ERHIShaderType::MS, defines);
            psoDesc.PS = pRenderer->GetShader("Model.hlsl", "PSMain", RHI::ERHIShaderType::PS, defines);
            psoDesc.RasterizerState.CullMode = m_bDoubleSided ? RHI::ERHICullMode::None : RHI::ERHICullMode::Back;
            psoDesc.RasterizerState.bFrontCCW = m_bFrontFaceCCW;
            psoDesc.DepthStencilState.bDepthTest = true;
            psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::GreaterEqual;
            // For now, we only implement forward rendering, so we only need one RT format
            psoDesc.RTFormats[0] = RHI::ERHIFormat::RGBA8SRGB;          // Diffuse RT
            // psoDesc.RTFormats[1] = RHI::ERHIFormat::RGBA8SRGB;          // Specular RT
            // psoDesc.RTFormats[2] = RHI::ERHIFormat::RGBA8UNORM;         // Normal RT
            // psoDesc.RTFormats[3] = RHI::ERHIFormat::R11G11B10F;         // Emissive RT
            // psoDesc.RTFormats[4] = RHI::ERHIFormat::RGBA32F;            // CustomDataRT
            psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

            m_pMeshletPSO = pRenderer->GetPipelineState(psoDesc, m_Name + "_ModelMeshletPSO");
        }
        return m_pMeshletPSO;
        // return nullptr;
    }

    RHI::RHIPipelineState *MeshMaterial::GetVertexSkinningPSO()
    {
        if (m_pVertexSkinningPSO == nullptr)
        {
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

            RHI::RHIComputePipelineStateDesc desc {};
            desc.CS = pRenderer->GetShader("VertexSkinning.hlsl", "CSMain", RHI::ERHIShaderType::CS);
            m_pVertexSkinningPSO = pRenderer->GetPipelineState(desc, m_Name + "_VertexSkinningPSO");
        }
        return m_pVertexSkinningPSO;
    }

    void MeshMaterial::UpdateConstants()
    {
        m_MaterialCB.ShadingModel = (uint)m_ShadingModel;
        m_MaterialCB.Albedo = m_AlbedoColor;
        m_MaterialCB.Emissive = m_EmissiveColor;
        m_MaterialCB.Metallic = m_Metallic;
        m_MaterialCB.Roughness = m_Roughness;
        m_MaterialCB.AlphaCutout = m_AlphaCutout;
        m_MaterialCB.Diffuse = m_DiffuseColor;
        m_MaterialCB.Specular = m_SpecularColor;
        m_MaterialCB.Glossiness = m_Glossiness;

        m_MaterialCB.bPBRMetallicRoughness = m_WorkFlow == MaterialWorkFlow::PBRMetallicRoughness;
        m_MaterialCB.bPBRSpecularGlossiness = m_WorkFlow == MaterialWorkFlow::PBRSpecularGlossiness;
        m_MaterialCB.bRGNormalTexture = m_pNormalTexture && (m_pNormalTexture->GetTexture()->GetDesc().Format == RHI::ERHIFormat::BC5UNORM);
        m_MaterialCB.bDoubleSided = m_bDoubleSided;
    }

    void MeshMaterial::OnGUI()
    {
    }

    void MeshMaterial::AddMaterialDefines(eastl::vector<eastl::string> &defines)
    {
        switch (m_ShadingModel)
        {
        case EShadingModel::DefaultPBR:
            defines.push_back("SHADING_MODEL_DEFAULT_PBR=1");
            break;
        default:
            break;
        };

        switch (m_WorkFlow)
        {
        case MaterialWorkFlow::PBRMetallicRoughness:
        {
            defines.push_back("WORKFLOW_METALLIC_ROUGHNESS=1");
            if (m_pAlbedoTexture)
            {
                defines.push_back("ALBEDO_TEXTURE=1");
            }
            if (m_pMetallicRoughTexture)
            {
                defines.push_back("METALLIC_ROUGHNESS_TEXTURE=1");
                if (m_pAOTexture == m_pMetallicRoughTexture)
                {
                    defines.push_back("AO_METALLIC_ROUGHNESS_TEXTURE=1");
                }
            }
            break;
        }
        case MaterialWorkFlow::PBRSpecularGlossiness:
        {
            defines.push_back("WORKFLOW_SPECULAR_GLOSSINESS=1");
            if (m_pDiffuseTexture)
            {
                defines.push_back("DIFFUSE_TEXTURE=1");
            }
            if (m_pSpecularGlossinessTexture)
            {
                defines.push_back("SPECULAR_GLOSSINESS_TEXTURE=1");
            }
            break;
        }
        default:
            break;
        }

        if (m_pNormalTexture)
        {
            defines.push_back("NORMAL_TEXTURE=1");
            if (m_pNormalTexture->GetTexture()->GetDesc().Format == RHI::ERHIFormat::BC5UNORM)
            {
                defines.push_back("RG_NORMAL_TEXTURE=1");
            }
        }

        if (m_bAlphaTest) defines.push_back("ALPHA_TEST=1");
        if (m_pEmissiveTexture) defines.push_back("EMISSIVE_TEXTURE=1");
        if (m_pAOTexture) defines.push_back("AO_TEXTURE=1");
        if (m_bDoubleSided) defines.push_back("DOUBLE_SIDED=1");
    }
}