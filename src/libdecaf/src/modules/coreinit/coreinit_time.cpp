#include "coreinit.h"
#include "coreinit_time.h"
#include "coreinit_systeminfo.h"
#include "common/platform_time.h"
#include "decaf_config.h"

namespace coreinit
{

static std::chrono::time_point<std::chrono::system_clock>
sEpochTime;


/**
 * Time since epoch
 */
OSTime
OSGetTime()
{
   auto now = std::chrono::system_clock::now();
   auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - sEpochTime);
   return static_cast<OSTime>(static_cast<double>(ns.count()) * decaf::config::system::time_scale);
}


/**
 * Time since system start up
 */
OSTime
OSGetSystemTime()
{
   return OSGetTime() - OSGetSystemInfo()->baseTime;
}


/**
 * Ticks since epoch
 */
OSTick
OSGetTick()
{
   return OSGetTime() & 0xFFFFFFFF;
}


/**
 * Ticks since system start up
 */
OSTick
OSGetSystemTick()
{
   return OSGetSystemTime() & 0xFFFFFFFF;
}


/**
 * Convert OSTime to OSCalendarTime
 */
void
OSTicksToCalendarTime(OSTime time,
                      OSCalendarTime *calendarTime)
{
   auto chrono = coreinit::internal::toTimepoint(time);
   std::time_t system_time_t = std::chrono::system_clock::to_time_t(chrono);
   std::tm tm = platform::localtime(system_time_t);

   calendarTime->tm_sec = tm.tm_sec;
   calendarTime->tm_min = tm.tm_min;
   calendarTime->tm_hour = tm.tm_hour;
   calendarTime->tm_mday = tm.tm_mday;
   calendarTime->tm_mon = tm.tm_mon;
   calendarTime->tm_year = tm.tm_year + 1900; // posix tm_year is year - 1900
}


/**
 * Convert OSCalendarTime to OSTime
 */
OSTime
OSCalendarTimeToTicks(OSCalendarTime *calendarTime)
{
   std::tm tm = { 0 };

   tm.tm_sec = calendarTime->tm_sec;
   tm.tm_min = calendarTime->tm_min;
   tm.tm_hour = calendarTime->tm_hour;
   tm.tm_mday = calendarTime->tm_mday;
   tm.tm_mon = calendarTime->tm_mon;
   tm.tm_year = calendarTime->tm_year - 1900;

   auto system_time = platform::make_gm_time(tm);
   auto chrono = std::chrono::system_clock::from_time_t(system_time);
   return coreinit::internal::toOSTime(chrono);
}

void
Module::initialiseClock()
{
   // Calculate the Wii U epoch (01/01/2000)
   std::tm tm = { 0 };
   tm.tm_sec = 0;
   tm.tm_min = 0;
   tm.tm_hour = 0;
   tm.tm_mday = 1;
   tm.tm_mon = 1;
   tm.tm_year = 100;
   tm.tm_isdst = -1;
   sEpochTime = std::chrono::system_clock::from_time_t(platform::make_gm_time(tm));
}

void
Module::registerTimeFunctions()
{
   RegisterKernelFunction(OSGetTime);
   RegisterKernelFunction(OSGetTick);
   RegisterKernelFunction(OSGetSystemTime);
   RegisterKernelFunction(OSGetSystemTick);
   RegisterKernelFunction(OSTicksToCalendarTime);
   RegisterKernelFunction(OSCalendarTimeToTicks);
}

namespace internal
{

std::chrono::time_point<std::chrono::system_clock>
toTimepoint(OSTime time)
{
   auto chrono = sEpochTime + std::chrono::nanoseconds(time);
   return std::chrono::time_point_cast<std::chrono::system_clock::duration>(chrono);
}

OSTime
toOSTime(std::chrono::time_point<std::chrono::system_clock> chrono)
{
   return std::chrono::duration_cast<std::chrono::nanoseconds>(chrono - sEpochTime).count();
}

} // namespace internal

} // namespace coreinit
