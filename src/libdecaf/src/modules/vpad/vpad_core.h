#pragma once
#include "common/types.h"

namespace vpad
{

void
VPADInit();

void
VPADSetAccParam(uint32_t chan, float unk1, float unk2);

void
VPADSetBtnRepeat(uint32_t chan, float unk1, float unk2);

} // namespace vpad
