#include "platform.h"
#include "platform_thread.h"

#ifdef PLATFORM_POSIX
#include <cstdlib>
#include <pthread.h>

namespace platform
{

void
setThreadName(std::thread *thread,
              const std::string &name)
{
#ifndef PLATFORM_APPLE
   auto handle = thread->native_handle();
   pthread_setname_np(handle, name.c_str());
#endif
}

void
exitThread(int result)
{
   auto res = reinterpret_cast<int *>(malloc(sizeof(int)));
   *res = result;
   pthread_exit(res);
}

} // namespace platform

#endif
