#pragma once
#include <common/cbool.h>

namespace cafe::coreinit
{

/**
* \defgroup coreinit_interrupts Interrupts
* \ingroup coreinit
* @{
*/

BOOL
OSEnableInterrupts();

BOOL
OSDisableInterrupts();

BOOL
OSRestoreInterrupts(BOOL enable);

BOOL
OSIsInterruptEnabled();

namespace internal
{

void
initialiseIci();

} // namespace internal

/** @} */

} // namespace cafe::coreinit
