#pragma once
#include "vpad_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::vpad
{

int32_t
VPADControlMotor(VPADChan chan,
                 virt_ptr<void> buffer,
                 uint32_t size);

void
VPADStopMotor(VPADChan chan);

} // namespace cafe::vpad
