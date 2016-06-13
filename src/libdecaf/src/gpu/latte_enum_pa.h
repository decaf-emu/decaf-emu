#pragma once
#include "common/types.h"

namespace latte
{

enum PA_FACE : uint32_t
{
   FACE_CCW = 0,
   FACE_CW  = 1
};

enum PA_PS_UCP_MODE : uint32_t
{
   PA_PS_UCP_CULL_DISTANCE       = 0,
   PA_PS_UCP_CULL_RADIUS         = 1,
   PA_PS_UCP_CULL_RADIUS_EXPAND  = 2,
   PA_PS_UCP_CULL_EXPAND         = 3,
};

} // namespace latte
