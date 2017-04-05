#pragma once

namespace coreinit
{

/**
 * \defgroup coreinit_appio App IO
 * \ingroup coreinit
 * @{
 */

struct OSMessageQueue;

OSMessageQueue *
OSGetDefaultAppIOQueue();

namespace internal
{

void
startAppIoThreads();

} // namespace internal

/** @} */

} // namespace coreinit
