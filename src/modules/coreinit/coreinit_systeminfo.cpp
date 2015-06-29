#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "coreinit_memory.h"
#include "coreinit_time.h"
#include <Windows.h>

int64_t
gEpochTime;

static p32<OSSystemInfo>
gSystemInfo;

static BOOL
gScreenCapturePermission;

p32<OSSystemInfo>
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

void
CoreInit::registerSystemInfoFunctions()
{
   RegisterSystemFunction(OSGetSystemInfo);
   RegisterSystemFunction(OSSetScreenCapturePermission);
   RegisterSystemFunction(OSGetScreenCapturePermission);
   RegisterSystemFunction(OSGetConsoleType);
}

void
CoreInit::initialiseSystemInformation()
{
   /*
   From a WiiU Console:
   uint32_t clockSpeed 248625000
   uint32_t 1243125000
   uint64_t baseTime 30373326953884705
   uint32_t 524288
   uint32_t 2097152
   uint32_t 524288
   uint32_t 1709422149632
   */

   // Setup gSystemInfo
   gSystemInfo = OSAllocFromSystem(sizeof(OSSystemInfo), 4);
   gSystemInfo->clockSpeed = static_cast<uint32_t>(CLOCK_SPEED);

   // Calculate the WiiU epoch (01/01/2000)
   SYSTEMTIME bt;
   FILETIME bft;
   bt.wYear = 2000;
   bt.wMonth = 1; // January
   bt.wDay = 1;
   bt.wDayOfWeek = 6; // Saturday
   bt.wHour = 0;
   bt.wMinute = 0;
   bt.wSecond = 0;
   bt.wMilliseconds = 0;
   SystemTimeToFileTime(&bt, &bft);

   gEpochTime = (static_cast<Time>(bft.dwHighDateTime) << 32) | bft.dwLowDateTime;

   // Get local time
   SYSTEMTIME lt;
   FILETIME lft;
   GetLocalTime(&lt);
   SystemTimeToFileTime(&lt, &lft);

   // Set baseTime as time since 01/01/2000
   auto localTime = (static_cast<Time>(lft.dwHighDateTime) << 32) | lft.dwLowDateTime;
   gSystemInfo->baseTime = localTime - gEpochTime;
}
