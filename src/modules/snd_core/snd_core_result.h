#pragma once
#include "types.h"

namespace snd_core
{

namespace AXResult_
{
enum Result : int32_t
{
   Success = 0,
   InvalidDeviceType = -1,
   InvalidDRCVSMode = -13,
};
}

using AXResult = AXResult_::Result;

} // namespace snd_core
