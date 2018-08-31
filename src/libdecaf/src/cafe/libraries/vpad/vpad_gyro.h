#pragma once
#include "vpad_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::vpad
{

float
VPADIsEnableGyroAccRevise(VPADChan chan);

float
VPADIsEnableGyroZeroPlay(VPADChan chan);

float
VPADIsEnableGyroZeroDrift(VPADChan chan);

float
VPADIsEnableGyroDirRevise(VPADChan chan);

} // namespace cafe::vpad
