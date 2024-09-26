#pragma once

enum class EShadingModel : uint
{
    DefaultPBR,
    Count,
};

inline float EncodeShadingModel(EShadingModel shadingModel)
{
    return (float)shadingModel / 255.0f;
}

inline EShadingModel DecodeShadingModel(float shadingModel)
{
    return (EShadingModel)round(shadingModel * 255.0f);
}