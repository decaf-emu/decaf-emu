#pragma once

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
#include "platform/platform_windows.h"
#else
#error Unsupported platform!
#endif

#include <ctime>

namespace platform {

tm localtime(const std::time_t& time)
{
   std::tm tm_snapshot;
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
   localtime_s(&tm_snapshot, &time);
#else
   localtime_r(&time, &tm_snapshot); // POSIX  
#endif
   return tm_snapshot;
}

}
