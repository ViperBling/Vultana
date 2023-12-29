struct FragmentInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

FragmentInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    FragmentInput result;
    result.position = position;
#if VULKAN
    result.position.y = -result.position.y;
#endif
    result.color = color;
    return result;
}

float4 PSMain(FragmentInput input) : SV_TARGET
{
    return input.color;
}