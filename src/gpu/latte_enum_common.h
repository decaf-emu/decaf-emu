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
   READ_256_BITS = 0,
   READ_512_BITS = 0,
};

} // namespace latte
