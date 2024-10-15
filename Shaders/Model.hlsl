#include "Common/Model.hlsli"
#include "Common/ShadingModel.hlsli"

FVertexOutput VSMain(uint vertexID : SV_VertexID)
{
    FVertexOutput vsOut = GetVertexOutput(cInstanceIndex, vertexID);
    return vsOut;
}

float4 PSMain(FVertexOutput psIn) : SV_Target
{
    return float4(psIn.TexCoord.xy, 0, 1.0f);
}