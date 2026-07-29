#include "Common/Model.hlsli"
#include "Common/ShadingModel.hlsli"

struct FGBufferOutput
{
    float4 Diffuse : SV_Target0;
    float4 Normal : SV_Target1;
    float2 Velocity : SV_Target2;
};


FVertexOutput VSMain(uint vertexID : SV_VertexID)
{
    FVertexOutput vsOut = GetVertexOutput(cInstanceIndex, vertexID);
    return vsOut;
}

FGBufferOutput PSMain(FVertexOutput psIn)
{
#if UNIFORM_RESOURCE
    uint instanceIndex = cInstanceIndex;
#else
    uint instanceIndex = psIn.InstanceIndex;
#endif

    FModelMaterialConstants material = GetMaterialConstants(instanceIndex);

    AlphaTest(instanceIndex, psIn.TexCoord);

#if ALBEDO_TEXTURE
    float4 mainTexVal = SampleMaterialTexture(material.AlbedoTexture, psIn.TexCoord, 0);
#elif DIFFUSE_TEXTURE
    float4 mainTexVal = SampleMaterialTexture(material.DiffuseTexture, psIn.TexCoord, 0);
#else
    float4 mainTexVal = 1.0f;
#endif

#if AO_TEXTURE
    float ao = SampleMaterialTexture(material.AmbientOcclusionTexture, psIn.TexCoord, 0).r;
#else
    float ao = 1.0f;
#endif

    float3 albedo = material.Albedo;
    float3 finalColor = mainTexVal.rgb * albedo;

    if (SceneCB.bShowMeshlets)
    {
        uint hash = WangHash(psIn.MeshletIndex);
        finalColor = float3(float(hash & 255), float((hash >> 8) & 255), float((hash >> 16) & 255)) / 255.0f;
    }

    float3 ndc = psIn.ClipPos.xyz / max(psIn.ClipPos.w, 0.0000001f);
    float3 prevNdc = psIn.PrevClipPos.xyz / max(psIn.PrevClipPos.w, 0.0000001f);
    float2 velocity = (ndc.xy - prevNdc.xy) * float2(0.5f, -0.5f);

    if (psIn.ClipPos.w <= 0.0f || psIn.PrevClipPos.w <= 0.0f)
    {
        velocity = float2(0.0f, 0.0f);
    }

    FGBufferOutput output = (FGBufferOutput)0;
    output.Diffuse = float4(finalColor * ao, 1.0f);
    output.Normal = float4(normalize(psIn.NormalWS) * 0.5f + 0.5f, 0.0f);
    output.Velocity = velocity;
    return output;
}