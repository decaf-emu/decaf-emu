#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_appio App IO
 * \ingroup coreinit
 * @{
 */

struct OSMessageQueue;

virt_ptr<OSMessageQueue>
OSGetDefaultAppIOQueue();

namespace internal
{

void
initialiseAppIoThreads();

} // namespace internal

/** @} */

} // namespace cafe::coreinit
