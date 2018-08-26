#pragma once
#include "coreinit_messagequeue.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_systemmessagequeue System Message Queue
 * \ingroup coreinit
 * @{
 */

virt_ptr<OSMessageQueue>
OSGetSystemMessageQueue();

namespace internal
{

void
initialiseSystemMessageQueue();

} // namespace internal

/** @} */

} // namespace cafe::coreinit
