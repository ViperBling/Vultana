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

    AlphaTest(cInstanceIndex, psIn.TexCoord);

#if ALBEDO_TEXTURE
    float4 albedo = SampleMaterialTexture(material.AlbedoTexture, psIn.TexCoord, 0);
#elif DIFFUSE_TEXTURE
    float4 albedo = SampleMaterialTexture(material.DiffuseTexture, psIn.TexCoord, 0);
#else
    float4 albedo = 0.0f;
#endif

#if AO_TEXTURE
    float ao = SampleMaterialTexture(material.AmbientOcclusionTexture, psIn.TexCoord, 0).r;
#else
    float ao = 1.0f;
#endif

    float3 finalColor = albedo.rgb;
    return float4(finalColor, 1.0f);
}