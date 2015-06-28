#include <ctime>
#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "coreinit_memory.h"

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
   gSystemInfo = OSAllocFromSystem(sizeof(OSSystemInfo), 4);
   gSystemInfo->clockSpeed = CLOCKS_PER_SEC * 4;

   /*
   From a WiiU Console:
   uint32_t 248625000
   uint32_t 1243125000
   uint64_t 30373326953884705
   uint32_t 524288
   uint32_t 2097152
   uint32_t 524288
   uint32_t 1709422149632
   */
}
