#include "platform.h"

#ifdef PLATFORM_WINDOWS
#include "platform_time.h"

namespace platform
{

tm
localtime(const std::time_t& time)
{
   std::tm tm_snapshot;
   localtime_s(&tm_snapshot, &time);
   return tm_snapshot;
}

time_t
make_gm_time(std::tm time)
{
   return _mkgmtime(&time);
}

} // namespace platform

#endif
