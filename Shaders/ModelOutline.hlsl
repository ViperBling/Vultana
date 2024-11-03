#include "Common/Model.hlsli"

struct FVSOutput
{
    float4 PositionCS : SV_POSITION;
#if ALPHA_TEST
    float2 TexCoord : TEXCOORD0;
#endif
};

FVSOutput VSMain(uint vertexID : SV_VertexID)
{
    FVertexAttributes vtx = GetVertexAttributes(cInstanceIndex, vertexID);
    FInstanceData instanceData = GetInstanceData(cInstanceIndex);

    float4 positionOS = float4(vtx.Position, 1.0f);
    float3 normalOS = vtx.Normal;

    float4 positionWS = mul(instanceData.MtxWorld, positionOS);
    float3 normalWS =  mul(instanceData.MtxWorldInverseTranspose, float4(normalOS, 0.0f)).xyz;

    FVSOutput vsOut = (FVSOutput)0;
    vsOut.PositionCS = mul(GetCameraConstants().MtxViewProjection, positionWS);
    
    float3 clipNormal = mul(GetCameraConstants().MtxViewProjection, float4(normalWS, 0.0f)).xyz * vsOut.PositionCS.w;

    const float width = 500;

    vsOut.PositionCS.xy += normalize(clipNormal.xy) * width * SceneCB.RenderSizeInv * clamp(vsOut.PositionCS.w, 0.0, 1.0);

#if ALPHA_TEST
    vsOut.TexCoord = vtx.TexCoord;
#endif

    return vsOut;
}

float4 PSMain(FVSOutput psIn) : SV_TARGET
{
#if ALPHA_TEST
    AlphaTest(cInstanceIndex, psIn.TexCoord);
#endif

    return float4(0.6, 0.4, 0.0, 1.0);
}