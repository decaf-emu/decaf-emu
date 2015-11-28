#include "platform.h"
#include "platform_time.h"

#ifdef PLATFORM_POSIX
#include <time.h>

namespace platform
{

tm
localtime(const std::time_t& time)
{
   std::tm tm_snapshot;
   localtime_r(&time, &tm_snapshot);
   return tm_snapshot;
}

time_t
make_gm_time(std::tm time)
{
   return timegm(&time);
}

}

#endif
