#include "coreinit.h"
#include "coreinit_time.h"
#include "coreinit_systeminfo.h"
#include <ctime>
#include <windows.h>

// Time since system start up
Time
OSGetSystemTime()
{
   SYSTEMTIME lt;
   FILETIME ft;
   GetLocalTime(&lt);
   SystemTimeToFileTime(&lt, &ft);

   Time base = OSGetSystemInfo()->baseTime;
   Time time = (static_cast<Time>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
   return (time - gEpochTime) - base;
}

// Time since 01/01/2000
Time
OSGetTime()
{
   Time base = OSGetSystemInfo()->baseTime;
   Time time = OSGetSystemTime();
   return time + base;
}

// Ticks since system start up
Ticks
OSGetSystemTick()
{
   return static_cast<Ticks>(OSGetSystemTime() & 0xffffffff);
}

Ticks
OSGetTick()
{
   auto base = OSGetSystemInfo()->baseTime;
   return OSGetSystemTick() + static_cast<Ticks>(base & 0xffffffff);
}

void
CoreInit::registerTimeFunctions()
{
   RegisterSystemFunction(OSGetTime);
   RegisterSystemFunction(OSGetTick);
   RegisterSystemFunction(OSGetSystemTime);
   RegisterSystemFunction(OSGetSystemTick);
}
