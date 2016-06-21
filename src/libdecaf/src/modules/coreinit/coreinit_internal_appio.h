#pragma once
#include "coreinit_messagequeue.h"

namespace coreinit
{

namespace internal
{

enum AppIoEventType : uint32_t
{
   FsAsyncCallback
};

void
sendMessage(OSMessage *message);

void
startAppIoThreads();

} // namespace internal

} // namespace coreinit
