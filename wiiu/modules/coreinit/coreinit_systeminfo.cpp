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

void
CoreInit::registerSystemInfoFunctions()
{
   RegisterSystemFunction(OSGetSystemInfo);
   RegisterSystemFunction(OSSetScreenCapturePermission);
   RegisterSystemFunction(OSGetScreenCapturePermission);
}

void
CoreInit::initialiseSystemInformation()
{
   gSystemInfo = OSAllocFromSystem(sizeof(OSSystemInfo), 4);
   gSystemInfo->clockSpeed = CLOCKS_PER_SEC * 4;
}
