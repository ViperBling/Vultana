cbuffer CB : register(b0)
{
    uint cResourceUAV;
};

cbuffer FloatValueCB : register(b1)
{
    float4 cClearValueFloat;
};

cbuffer UintValueCB : register(b2)
{
    uint4 cClearValueUint;
};

#if defined(UAV_TYPE_FLOAT)
    #define UAV_TYPE float
    #define CLEAR_VALUE cClearValueFloat.x
#elif defined(UAV_TYPE_FLOAT2)
    #define UAV_TYPE float2
    #define CLEAR_VALUE cClearValueFloat.xy
#elif defined(UAV_TYPE_FLOAT3)
    #define UAV_TYPE float3
    #define CLEAR_VALUE cClearValueFloat.xyz
#elif defined(UAV_TYPE_FLOAT4)
    #define UAV_TYPE float4
    #define CLEAR_VALUE cClearValueFloat.xyzw
#elif defined(UAV_TYPE_UINT)
    #define UAV_TYPE uint
    #define CLEAR_VALUE cClearValueUint.x
#elif defined(UAV_TYPE_UINT2)
    #define UAV_TYPE uint2
    #define CLEAR_VALUE cClearValueUint.xy
#elif defined(UAV_TYPE_UINT3)
    #define UAV_TYPE uint3
    #define CLEAR_VALUE cClearValueUint.xyz
#elif defined(UAV_TYPE_UINT4)
    #define UAV_TYPE uint4
    #define CLEAR_VALUE cClearValueUint.xyzw
#else
    #error "unsupported uav type"
#endif

[numthreads(8, 8, 1)]
void ClearUAV_Texture2D(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture2D<UAV_TYPE> uav = ResourceDescriptorHeap[cResourceUAV];
    uav[dispatchThreadID] = CLEAR_VALUE;
}

[numthreads(8, 8, 1)]
void ClearUAV_Texture2D_Array(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture2DArray<UAV_TYPE> uav = ResourceDescriptorHeap[cResourceUAV];
    uav[dispatchThreadID] = CLEAR_VALUE;    
}

[numthreads(8, 8, 8)]
void ClearUAV_Texture3D(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture3D<UAV_TYPE> uav = ResourceDescriptorHeap[cResourceUAV];
    uav[dispatchThreadID] = CLEAR_VALUE;
}

[numthreads(64, 1, 1)]
void ClearUAV_Typed_Buffer(uint dispatchThreadID : SV_DispatchThreadID)
{
    RWBuffer<UAV_TYPE> uav = ResourceDescriptorHeap[cResourceUAV];
    uav[dispatchThreadID] = CLEAR_VALUE;
}

[numthreads(64, 1, 1)]
void ClearUAV_Raw_Buffer(uint dispatchThreadID : SV_DispatchThreadID)
{
    RWByteAddressBuffer uav = ResourceDescriptorHeap[cResourceUAV];
    uav.Store<UAV_TYPE>(sizeof(UAV_TYPE) * dispatchThreadID, CLEAR_VALUE);
}