#include "platform_time.h"

#ifdef PLATFORM_POSIX

namespace platform
{

tm localtime(const std::time_t& time)
{
   std::tm tm_snapshot;
   localtime_r(&time, &tm_snapshot);
   return tm_snapshot;
}

}

#endif
