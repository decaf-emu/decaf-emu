#pragma once
#include "types.h"

namespace AXResult
{
enum Result : int32_t
{
   Success = 0,
   InvalidDeviceType = -1,
   InvalidDRCVSMode = -13,
};
}
