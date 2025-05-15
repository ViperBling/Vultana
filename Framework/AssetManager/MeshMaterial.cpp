#include "MeshMaterial.hpp"
#include "ResourceCache.hpp"
#include "Core/VultanaEngine.hpp"

namespace Assets
{
    MeshMaterial::~MeshMaterial()
    {
        auto resourceCache = ResourceCache::GetInstance();
        resourceCache->ReleaseTexture2D(mpDiffuseTexture);
        resourceCache->ReleaseTexture2D(mpSpecularGlossinessTexture);
        resourceCache->ReleaseTexture2D(mpAlbedoTexture);
        resourceCache->ReleaseTexture2D(mpMetallicRoughTexture);
        resourceCache->ReleaseTexture2D(mpNormalTexture);
        resourceCache->ReleaseTexture2D(mpEmissiveTexture);
        resourceCache->ReleaseTexture2D(mpAOTexture);
    }

    RHI::RHIPipelineState *MeshMaterial::GetPSO()
    {
        if (mpPSO == nullptr)
        {
            Renderer::RendererBase* pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

            eastl::vector<eastl::string> defines;
            AddMaterialDefines(defines);
            defines.push_back("UNIFORM_RESOURCE=1");

            if (mpAlbedoTexture) defines.push_back("ALBEDO_TEXTURE=1");
            if (mpDiffuseTexture) defines.push_back("DIFFUSE_TEXTURE=1");
            if (mbAlphaTest) defines.push_back("ALPHA_TEST=1");

            RHI::RHIGraphicsPipelineStateDesc psoDesc {};
            psoDesc.VS = pRenderer->GetShader("Model.hlsl", "VSMain", RHI::ERHIShaderType::VS, defines);
            psoDesc.PS = pRenderer->GetShader("Model.hlsl", "PSMain", RHI::ERHIShaderType::PS, defines);
            psoDesc.RasterizerState.CullMode = mbDoubleSided ? RHI::ERHICullMode::None : RHI::ERHICullMode::Back;
            psoDesc.RasterizerState.bFrontCCW = mbFrontFaceCCW;
            psoDesc.DepthStencilState.bDepthTest = true;
            psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::GreaterEqual;
            // For now, we only implement forward rendering, so we only need one RT format
            psoDesc.RTFormats[0] = RHI::ERHIFormat::RGBA8SRGB;          // Diffuse RT
            // psoDesc.RTFormats[1] = RHI::ERHIFormat::RGBA8SRGB;          // Specular RT
            // psoDesc.RTFormats[2] = RHI::ERHIFormat::RGBA8UNORM;         // Normal RT
            // psoDesc.RTFormats[3] = RHI::ERHIFormat::R11G11B10F;         // Emissive RT
            // psoDesc.RTFormats[4] = RHI::ERHIFormat::RGBA32F;            // CustomDataRT
            psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

            mpPSO = pRenderer->GetPipelineState(psoDesc, mName + "_ModelPSO");
        }
        return mpPSO;
    }

    RHI::RHIPipelineState *MeshMaterial::GetIDPSO()
    {
        if (mpIDPSO == nullptr)
        {
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

            eastl::vector<eastl::string> defines;
            defines.push_back("UNIFORM_RESOURCE=1");

            if (mpAlbedoTexture) defines.push_back("ALBEDO_TEXTURE=1");
            if (mpDiffuseTexture) defines.push_back("DIFFUSE_TEXTURE=1");
            if (mbAlphaTest) defines.push_back("ALPHA_TEST=1");

            RHI::RHIGraphicsPipelineStateDesc psoDesc {};
            psoDesc.VS = pRenderer->GetShader("ModelID.hlsl", "VSMain", RHI::ERHIShaderType::VS, defines);
            psoDesc.PS = pRenderer->GetShader("ModelID.hlsl", "PSMain", RHI::ERHIShaderType::PS, defines);
            psoDesc.RasterizerState.CullMode = mbDoubleSided ? RHI::ERHICullMode::None : RHI::ERHICullMode::Back;
            psoDesc.RasterizerState.bFrontCCW = mbFrontFaceCCW;
            psoDesc.DepthStencilState.bDepthTest = true;
            psoDesc.DepthStencilState.bDepthWrite = false;
            psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::GreaterEqual;
            psoDesc.RTFormats[0] = RHI::ERHIFormat::R32UI;
            psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

            mpIDPSO = pRenderer->GetPipelineState(psoDesc, mName + "_ModelIDPSO");
        }
        return mpIDPSO;
    }

    RHI::RHIPipelineState *MeshMaterial::GetOutlinePSO()
    {
        if (mpOutlinePSO == nullptr)
        {
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

            eastl::vector<eastl::string> defines;
            defines.push_back("UNIFORM_RESOURCE=1");
            
            if (mbAlphaTest) defines.push_back("ALPHA_TEST=1");

            RHI::RHIGraphicsPipelineStateDesc psoDesc {};
            psoDesc.VS = pRenderer->GetShader("ModelOutline.hlsl", "VSMain", RHI::ERHIShaderType::VS, defines);
            psoDesc.PS = pRenderer->GetShader("ModelOutline.hlsl", "PSMain", RHI::ERHIShaderType::PS, defines);
            psoDesc.RasterizerState.CullMode = RHI::ERHICullMode::Front;
            psoDesc.RasterizerState.bFrontCCW = mbFrontFaceCCW;
            psoDesc.DepthStencilState.bDepthTest = true;
            psoDesc.DepthStencilState.bDepthWrite = false;
            psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::GreaterEqual;
            psoDesc.RTFormats[0] = RHI::ERHIFormat::RGBA8SRGB;
            psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

            mpOutlinePSO = pRenderer->GetPipelineState(psoDesc, mName + "_ModelOutlinePSO");
        }
        return mpOutlinePSO;
    }

    RHI::RHIPipelineState *MeshMaterial::GetMeshletPSO()
    {
        if (mpMeshletPSO == nullptr)
        {
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

            eastl::vector<eastl::string> defines;
            AddMaterialDefines(defines);

            RHI::RHIMeshShadingPipelineStateDesc psoDesc {};
            psoDesc.AS = pRenderer->GetShader("MeshletCulling.hlsl", "ASMain", RHI::ERHIShaderType::AS, defines);
            psoDesc.MS = pRenderer->GetShader("ModelMeshlet.hlsl", "MSMain", RHI::ERHIShaderType::MS, defines);
            psoDesc.PS = pRenderer->GetShader("Model.hlsl", "PSMain", RHI::ERHIShaderType::PS, defines);
            psoDesc.RasterizerState.CullMode = mbDoubleSided ? RHI::ERHICullMode::None : RHI::ERHICullMode::Back;
            psoDesc.RasterizerState.bFrontCCW = mbFrontFaceCCW;
            psoDesc.DepthStencilState.bDepthTest = true;
            psoDesc.DepthStencilState.DepthFunc = RHI::RHICompareFunc::GreaterEqual;
            // For now, we only implement forward rendering, so we only need one RT format
            psoDesc.RTFormats[0] = RHI::ERHIFormat::RGBA8SRGB;          // Diffuse RT
            // psoDesc.RTFormats[1] = RHI::ERHIFormat::RGBA8SRGB;          // Specular RT
            // psoDesc.RTFormats[2] = RHI::ERHIFormat::RGBA8UNORM;         // Normal RT
            // psoDesc.RTFormats[3] = RHI::ERHIFormat::R11G11B10F;         // Emissive RT
            // psoDesc.RTFormats[4] = RHI::ERHIFormat::RGBA32F;            // CustomDataRT
            psoDesc.DepthStencilFormat = RHI::ERHIFormat::D32F;

            mpMeshletPSO = pRenderer->GetPipelineState(psoDesc, mName + "_ModelMeshletPSO");
        }
        return mpMeshletPSO;
        // return nullptr;
    }

    RHI::RHIPipelineState *MeshMaterial::GetVertexSkinningPSO()
    {
        if (mpVertexSkinningPSO == nullptr)
        {
            auto pRenderer = Core::VultanaEngine::GetEngineInstance()->GetRenderer();

            RHI::RHIComputePipelineStateDesc desc {};
            desc.CS = pRenderer->GetShader("VertexSkinning.hlsl", "CSMain", RHI::ERHIShaderType::CS);
            mpVertexSkinningPSO = pRenderer->GetPipelineState(desc, mName + "_VertexSkinningPSO");
        }
        return mpVertexSkinningPSO;
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

    void MeshMaterial::AddMaterialDefines(eastl::vector<eastl::string> &defines)
    {
        switch (mShadingModel)
        {
        case EShadingModel::DefaultPBR:
            defines.push_back("SHADING_MODEL_DEFAULT_PBR=1");
            break;
        default:
            break;
        };

        switch (mWorkFlow)
        {
        case MaterialWorkFlow::PBRMetallicRoughness:
        {
            defines.push_back("WORKFLOW_METALLIC_ROUGHNESS=1");
            if (mpAlbedoTexture)
            {
                defines.push_back("ALBEDO_TEXTURE=1");
            }
            if (mpMetallicRoughTexture)
            {
                defines.push_back("METALLIC_ROUGHNESS_TEXTURE=1");
                if (mpAOTexture == mpMetallicRoughTexture)
                {
                    defines.push_back("AO_METALLIC_ROUGHNESS_TEXTURE=1");
                }
            }
            break;
        }
        case MaterialWorkFlow::PBRSpecularGlossiness:
        {
            defines.push_back("WORKFLOW_SPECULAR_GLOSSINESS=1");
            if (mpDiffuseTexture)
            {
                defines.push_back("DIFFUSE_TEXTURE=1");
            }
            if (mpSpecularGlossinessTexture)
            {
                defines.push_back("SPECULAR_GLOSSINESS_TEXTURE=1");
            }
            break;
        }
        default:
            break;
        }

        if (mpNormalTexture)
        {
            defines.push_back("NORMAL_TEXTURE=1");
            if (mpNormalTexture->GetTexture()->GetDesc().Format == RHI::ERHIFormat::BC5UNORM)
            {
                defines.push_back("RG_NORMAL_TEXTURE=1");
            }
        }

        if (mbAlphaTest) defines.push_back("ALPHA_TEST=1");
        if (mpEmissiveTexture) defines.push_back("EMISSIVE_TEXTURE=1");
        if (mpAOTexture) defines.push_back("AO_TEXTURE=1");
        if (mbDoubleSided) defines.push_back("DOUBLE_SIDED=1");
    }
}