#include "Common/ModelConstants.hlsli"

struct FVertexOutput
{
    float4 positionCS : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normalOS : NORMAL;
    float3 tangentOS : TANGENT;
    float3 positionWS : TEXCOORD1;
};

