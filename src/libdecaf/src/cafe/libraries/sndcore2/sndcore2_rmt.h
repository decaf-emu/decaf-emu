#pragma once
#include "sndcore2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::sndcore2
{

int32_t
AXRmtGetSamplesLeft();

int32_t
AXRmtGetSamples(int32_t,
                virt_ptr<uint8_t> buffer,
                int32_t samples);

int32_t
AXRmtAdvancePtr(int32_t numSamples);

} // namespace cafe::sndcore2
