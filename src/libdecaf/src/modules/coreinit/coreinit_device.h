#pragma once
#include "common/types.h"

namespace coreinit
{

/**
 * \defgroup coreinit_device Device
 * \ingroup coreinit
 * @{
 */

uint16_t
OSReadRegister16(uint32_t device, uint32_t id);

uint32_t
OSReadRegister32Ex(uint32_t device, uint32_t id);

void
OSWriteRegister32Ex(uint32_t device, uint32_t id, uint32_t value);

/** @} */

} // namespace coreinit
