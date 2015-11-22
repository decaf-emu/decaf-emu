#pragma once
#include "types.h"

namespace latte
{

enum DB_DEPTH_FORMAT : uint32_t
{
   DEPTH_INVALID           = 0,
   DEPTH_16                = 1,
   DEPTH_X8_24             = 2,
   DEPTH_8_24              = 3,
   DEPTH_X8_24_FLOAT       = 4,
   DEPTH_8_24_FLOAT        = 5,
   DEPTH_32_FLOAT          = 6,
   DEPTH_X24_8_32_FLOAT    = 7,
};

enum DB_STENCIL_FUNC : uint32_t
{
   DB_STENCIL_KEEP         = 0,
   DB_STENCIL_ZERO         = 1,
   DB_STENCIL_REPLACE      = 2,
   DB_STENCIL_INCR_CLAMP   = 3,
   DB_STENCIL_DECR_CLAMP   = 4,
   DB_STENCIL_INVERT       = 5,
   DB_STENCIL_INCR_WRAP    = 6,
   DB_STENCIL_DECR_WRAP    = 7,
};

enum DB_Z_ORDER
{
   DB_LATE_Z               = 0,
   DB_EARLY_Z_THEN_LATE_Z  = 1,
   DB_RE_Z                 = 2,
   DB_EARLY_Z_THEN_RE_Z    = 3,
};

} // namespace latte
