#pragma once

#include "RHI/RHI.hpp"

namespace Scene
{
    class TextureLoader
    {

    private:
        uint32_t mWidth = 1;
        uint32_t mHeight = 1;
        uint32_t mDepth = 1;
        uint32_t mMipLevels = 1;
        uint32_t mArraySize = 1;
        RHI::ERHIFormat mFormat = RHI::ERHIFormat::Unknown;
    };
}