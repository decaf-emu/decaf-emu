#pragma once
#include "types.h"

namespace snd_core
{

namespace AXResult
{
enum Result : int32_t
{
   Success = 0,
   InvalidDeviceType = -1,
   InvalidDRCVSMode = -13,
};
}

} // namespace snd_core
