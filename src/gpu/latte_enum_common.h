#pragma once
#include "types.h"

namespace latte
{

enum ARRAY_MODE : uint32_t
{
   ARRAY_LINEAR_GENERAL = 0,
   ARRAY_LINEAR_ALIGNED = 1,
   ARRAY_2D_TILED_THIN1 = 4,
};

enum READ_SIZE : uint32_t
{
   READ_256_BITS        = 0,
   READ_512_BITS        = 0,
};

enum REF_FUNC : uint32_t
{
   REF_NEVER            = 0,
   REF_LESS             = 1,
   REF_EQUAL            = 2,
   REF_LEQUAL           = 3,
   REF_GREATER          = 4,
   REF_NOTEQUAL         = 5,
   REF_GEQUAL           = 6,
   REF_ALWAYS           = 7,
};

} // namespace latte
