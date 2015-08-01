#include "coreinit.h"
#include "coreinit_time.h"
#include "coreinit_systeminfo.h"

// Time since epoch
OSTime
OSGetTime()
{
   auto now = std::chrono::system_clock::now();
   auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - gEpochTime);
   return ns.count();
}

// Time since system start up
OSTime
OSGetSystemTime()
{
   return OSGetTime() - OSGetSystemInfo()->baseTime;
}

// Ticks since epoch
OSTick
OSGetTick()
{
   return OSGetTime() & 0xFFFFFFFF;
}

// Ticks since system start up
OSTick
OSGetSystemTick()
{
   return OSGetSystemTime() & 0xFFFFFFFF;
}

std::chrono::time_point<std::chrono::system_clock>
OSTimeToChrono(OSTime time)
{
   auto chrono = gEpochTime + std::chrono::nanoseconds(time);
   return std::chrono::time_point_cast<std::chrono::system_clock::duration>(chrono);
}

void
OSTicksToCalendarTime(OSTime time, OSCalendarTime* calendarTime)
{
   auto chrono = OSTimeToChrono(time);
   std::time_t system_time_t = std::chrono::system_clock::to_time_t(chrono);
   std::tm* tm = std::localtime(&system_time_t);

   calendarTime->tm_sec = tm->tm_sec;
   calendarTime->tm_min = tm->tm_min;
   calendarTime->tm_hour = tm->tm_hour;
   calendarTime->tm_mday = tm->tm_mday;
   calendarTime->tm_mon = tm->tm_mon;
   calendarTime->tm_year = tm->tm_year + 1900; // posix tm_year is year - 1900
}

void
CoreInit::registerTimeFunctions()
{
   RegisterKernelFunction(OSGetTime);
   RegisterKernelFunction(OSGetTick);
   RegisterKernelFunction(OSGetSystemTime);
   RegisterKernelFunction(OSGetSystemTick);
   RegisterKernelFunction(OSTicksToCalendarTime);
}
