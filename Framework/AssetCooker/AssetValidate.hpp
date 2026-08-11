// FAssetValidate.hpp
#pragma once

#include "AssetCooker/VAssetTypes.hpp"

#include <EASTL/string.h>

namespace Assets
{
    bool ValidateModel(const FModelCookedData& payload, const eastl::string& context);
    bool ValidateTexture(const FTextureCookedData& payload, const eastl::string& context);
}