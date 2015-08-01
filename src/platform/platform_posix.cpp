#include "../platform.h"
#ifdef PLATFORM_POSIX

#include <ctime>
#include <thread>

namespace platform {

tm localtime(const std::time_t& time)
{
   std::tm tm_snapshot;
   localtime_r(&time, &tm_snapshot); // POSIX  
   return tm_snapshot;
}

void set_thread_name(std::thread* thread, thread, const std::string& threadName)
{
   auto handle = thread->native_handle();
   pthread_setname_np(handle, threadName.c_str());
}

}

#endif
