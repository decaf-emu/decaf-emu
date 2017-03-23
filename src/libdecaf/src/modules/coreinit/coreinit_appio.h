#pragma once

namespace coreinit
{

struct OSMessageQueue;

OSMessageQueue *
OSGetDefaultAppIOQueue();

namespace internal
{

void
startAppIoThreads();

} // namespace internal

} // namespace coreinit
