#include "platform_time.h"

#ifdef PLATFORM_WINDOWS

namespace platform
{

tm localtime(const std::time_t& time)
{
   std::tm tm_snapshot;
   localtime_s(&tm_snapshot, &time);
   return tm_snapshot;
}

} // namespace platform

#endif
