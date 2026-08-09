#pragma once

#include "AssetCooker/VAssetTypes.hpp"
#include "AssetManager/MeshMaterial.hpp"
#include "Scene/SceneComponent/Animation.hpp"
#include "RHI/RHICommon.hpp"

namespace Assets
{
    inline MaterialWorkFlow ToEngineWorkFlow(EMaterialWorkFlow workflow)
    {
        switch (workflow)
        {
        case EMaterialWorkFlow::PBRSpecularGlossiness:
            return MaterialWorkFlow::PBRSpecularGlossiness;
        case EMaterialWorkFlow::PBRMetallicRoughness:
        default:
            return MaterialWorkFlow::PBRMetallicRoughness;
        }
    }

    // IndexStride is {1,2,4} per ValidateModel; cook widened 1->16 bit, so effectively {2,4}.
    inline RHI::ERHIFormat ToIndexFormat(uint32_t stride)
    {
        return stride == 4 ? RHI::ERHIFormat::R32UI : RHI::ERHIFormat::R16UI;
    }

    inline Scene::EAnimationChannelMode ToChannelMode(uint32_t diskMode)
    {
        return static_cast<Scene::EAnimationChannelMode>(diskMode);
    }
}