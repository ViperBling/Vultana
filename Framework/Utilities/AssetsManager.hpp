#pragma once

#include <iostream>
#include <cassert>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

std::string GetAssetsPath()
{
    return ASSETS_DIR;
    // return getenv("ASSETS_DIR");
}

namespace Vultana
{
    
}