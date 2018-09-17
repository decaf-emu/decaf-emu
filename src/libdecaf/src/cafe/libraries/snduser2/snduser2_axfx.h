#pragma once
#include "snduser2_enum.h"

#include "cafe/libraries/sndcore2/sndcore2_device.h"

#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

using AXAuxCallbackData = sndcore2::AXAuxCallbackData;

struct AXFXBuffers
{
   virt_ptr<int32_t> left;
   virt_ptr<int32_t> right;
   virt_ptr<int32_t> surround;
};
CHECK_OFFSET(AXFXBuffers, 0x00, left);
CHECK_OFFSET(AXFXBuffers, 0x04, right);
CHECK_OFFSET(AXFXBuffers, 0x08, surround);
CHECK_SIZE(AXFXBuffers, 0x0C);

} // namespace cafe::snduser2
