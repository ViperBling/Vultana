#include "Common/Model.hlsli"
#include "Common/ShadingModel.hlsli"

FVertexOutput VSMain(uint vertexID : SV_VertexID)
{
    FVertexOutput vsOut = GetVertexOutput(cInstanceIndex, vertexID);
    return vsOut;
}

float4 PSMain(FVertexOutput psIn) : SV_Target
{
    FModelMaterialConstants material = GetMaterialConstants(cInstanceIndex);
#if ALBEDO_TEXTURE
    float4 albedo = SampleMaterialTexture(material.AlbedoTexture, psIn.TexCoord, 0);
#elif DIFFUSE_TEXTURE
    float4 albedo = SampleMaterialTexture(material.DiffuseTexture, psIn.TexCoord, 0);
#else
    float4 albedo = 0.0f;
#endif

    return float4(albedo.rgb, 1.0f);
}