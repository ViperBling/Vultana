#pragma once

#include <stdint.h>

namespace RG
{
    struct RGHandle
    {
        uint16_t Index = uint16_t(-1);
        uint16_t Node = uint16_t(-1);

        bool IsValid() const { return Index != uint16_t(-1) && Node != uint16_t(-1); }
    };
}