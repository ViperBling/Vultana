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

    float4 positionOS = float4(vtx.Position, 1.0f);
    float4 positionWS = mul(GetInstanceData(cInstanceIndex).MtxWorld, positionOS);

    FVSOutput vsOut = (FVSOutput)0;
    vsOut.PositionCS = mul(GetCameraConstants().MtxViewProjection, positionWS);

#if ALPHA_TEST
    vsOut.TexCoord = vtx.TexCoord;
#endif

    return vsOut;
}

uint PSMain(FVSOutput psIn) : SV_TARGET
{
#if ALPHA_TEST
    AlphaTest(cInstanceIndex, psIn.TexCoord);
#endif

    return GetInstanceData(cInstanceIndex).ObjectID;
}