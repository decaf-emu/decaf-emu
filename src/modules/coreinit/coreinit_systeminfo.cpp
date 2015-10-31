#include <chrono>
#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "coreinit_memheap.h"
#include "coreinit_time.h"
#include "platform/platform_time.h"

std::chrono::time_point<std::chrono::system_clock>
gEpochTime;

OSSystemInfo *
gSystemInfo = nullptr;

static BOOL
gScreenCapturePermission = TRUE;

static BOOL
gEnableHomeButtonMenu = FALSE;

OSSystemInfo *
OSGetSystemInfo()
{
   return gSystemInfo;
}

BOOL
OSSetScreenCapturePermission(BOOL enabled)
{
   auto old = gScreenCapturePermission;
   gScreenCapturePermission = enabled;
   return old;
}

BOOL
OSGetScreenCapturePermission()
{
   return gScreenCapturePermission;
}

uint32_t
OSGetConsoleType()
{
   // Value from a WiiU retail v4.0.0
   return 0x3000050;
}

BOOL
OSEnableHomeButtonMenu(BOOL enable)
{
   gEnableHomeButtonMenu = enable;
   return TRUE;
}

void
OSBlockThreadsOnExit()
{
   // TODO: OSBlockThreadsOnExit
}

void
CoreInit::registerSystemInfoFunctions()
{
   RegisterKernelFunction(OSGetSystemInfo);
   RegisterKernelFunction(OSSetScreenCapturePermission);
   RegisterKernelFunction(OSGetScreenCapturePermission);
   RegisterKernelFunction(OSGetConsoleType);
   RegisterKernelFunction(OSEnableHomeButtonMenu);
   RegisterKernelFunction(OSBlockThreadsOnExit);
}

void
CoreInit::initialiseSystemInformation()
{
   // Setup gSystemInfo
   gSystemInfo = OSAllocFromSystem<OSSystemInfo>();

   // Clockspeed is 4 * 1 second in ticks
   auto oneSecond = std::chrono::seconds(1);
   auto oneSecondNS = std::chrono::duration_cast<std::chrono::nanoseconds>(oneSecond);
   gSystemInfo->clockSpeed = oneSecondNS.count() * 4;

   // Calculate the Wii U epoch (01/01/2000)
   std::tm tm = { 0 };
   tm.tm_sec = 0;
   tm.tm_min = 0;
   tm.tm_hour = 0;
   tm.tm_mday = 1;
   tm.tm_mon = 1;
   tm.tm_year = 100;
   tm.tm_isdst = -1;
   gEpochTime = std::chrono::system_clock::from_time_t(platform::make_gm_time(tm));

   // Calculate base time
   auto now = std::chrono::system_clock::now();
   auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - gEpochTime);
   gSystemInfo->baseTime = ns.count();
}
