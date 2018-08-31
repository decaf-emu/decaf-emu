#pragma once
#include "coreinit_enum.h"

#include <cstdint>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_device Device
 * \ingroup coreinit
 * @{
 */

uint16_t
OSReadRegister16(OSDeviceID device,
                 uint32_t id);

void
OSWriteRegister16(OSDeviceID device,
                  uint32_t id,
                  uint16_t value);

uint32_t
OSReadRegister32Ex(OSDeviceID device,
                   uint32_t id);

void
OSWriteRegister32Ex(OSDeviceID device,
                    uint32_t id,
                    uint32_t value);

namespace internal
{

void
initialiseDeviceTable();

} // namespace internal

/** @} */

} // namespace cafe::coreinit
