#include "Common/Common.hlsli"

cbuffer SPDConstants : register(b1)
{
    uint    cMips;
    uint    cNumWorkGroups;
    uint2   cWorkGroupOffset;

    float2  cInvInputSize;
    uint    cImgSrc;
    uint    cSpdGlobalAtomicUAV;

    uint4   cImgDst[12];
};

#define A_GPU
#define A_HLSL

#include "Common/FFX_A.hpp"

groupshared AU1 spdCounter;
groupshared AF1 spdIntermediateR[16][16];

AF4 SpdLoadSourceImage(ASU2 p, AU1 slice)
{
    Texture2D imgSrc = ResourceDescriptorHeap[cImgSrc];
    AF2 textureCoord = p * cInvInputSize + cInvInputSize;

    SamplerState pointClampSampler = SamplerDescriptorHeap[SceneCB.PointClampSampler];
    float4 R = imgSrc.GatherRed(pointClampSampler, textureCoord);
    float minValue = min(min(R.x, R.y), min(R.z, R.w));

    return AF4(minValue, 0, 0, 0);
}

AF4 SpdLoad(ASU2 tex, AU1 slice)
{
    globallycoherent RWTexture2D<float> imgDst5 = ResourceDescriptorHeap[cImgDst[5].x];
    return float4(imgDst5[tex], 0, 0, 0);
}

void SpdStore(ASU2 pix, AF4 outValue, AU1 index, AU1 slice)
{
    if (index == 5)
    {
        globallycoherent RWTexture2D<float> imgDst5 = ResourceDescriptorHeap[cImgDst[5].x];
        imgDst5[pix] = outValue.x;
        return;
    }

    RWTexture2D<float> imgDst = ResourceDescriptorHeap[cImgDst[index].x];
    imgDst[pix] = outValue.x;
}

void SpdIncreaseAtomicCounter(AU1 slice)
{
    globallycoherent RWBuffer<uint> spdGlobalAtomic = ResourceDescriptorHeap[cSpdGlobalAtomicUAV];
    InterlockedAdd(spdGlobalAtomic[0], 1, spdCounter);
}

AU1 SpdGetAtomicCounter()
{
    return spdCounter;
}

void SpdResetAtomicCounter(AU1 slice)
{
    globallycoherent RWBuffer<uint> spdGlobalAtomic = ResourceDescriptorHeap[cSpdGlobalAtomicUAV];
    spdGlobalAtomic[0] = 0;
}

AF4 SpdLoadIntermediate(AU1 x, AU1 y)
{
    return AF4(spdIntermediateR[x][y], 0, 0, 0);
}

void SpdStoreIntermediate(AU1 x, AU1 y, AF4 value)
{
    spdIntermediateR[x][y] = value.x;
}

AF4 SpdReduce4(AF4 v0, AF4 v1, AF4 v2, AF4 v3)
{
    return float4(min(min(v0.x, v1.x), min(v2.x, v3.x)), 0, 0, 0);
}

#define SPD_LINEAR_SAMPLER

#include "Common/FFX_SPD.hpp"

[numthreads(256, 1, 1)]
void BuildHZBCS(uint3 WorkGroupId : SV_GroupID, uint LocalThreadIndex : SV_GroupIndex)
{
    SpdDownsample(
        AU2(WorkGroupId.xy),
        AU1(LocalThreadIndex),
        AU1(cMips),
        AU1(cNumWorkGroups),
        AU1(WorkGroupId.z),
        AU2(cWorkGroupOffset));
}