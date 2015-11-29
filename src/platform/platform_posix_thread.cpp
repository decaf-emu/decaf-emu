#include "platform.h"
#include "platform_thread.h"

#ifdef PLATFORM_POSIX
#include <pthread.h>

namespace platform
{

void setThreadName(std::thread *thread, const std::string &name)
{
   auto handle = thread->native_handle();
   pthread_setname_np(handle, name.c_str());
}

} // namespace platform

#endif
