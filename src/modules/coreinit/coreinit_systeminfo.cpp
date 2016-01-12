#include <chrono>
#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "coreinit_memheap.h"
#include "coreinit_time.h"
#include "platform/platform_time.h"

static OSSystemInfo *
gSystemInfo = nullptr;

static BOOL
gScreenCapturePermission = TRUE;

static BOOL
gEnableHomeButtonMenu = FALSE;

static uint64_t
gTitleID = 0;

static uint64_t
gSystemID = 0x000500101000400Aull;

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

uint64_t
OSGetTitleID()
{
   return gTitleID;
}

uint64_t
OSGetOSID()
{
   return gSystemID;
}

void
CoreInit::initialiseSystemInformation()
{
   // Setup gSystemInfo
   gSystemInfo = coreinit::internal::sysAlloc<OSSystemInfo>();

   // Clockspeed is 4 * 1 second in ticks
   auto oneSecond = std::chrono::seconds(1);
   auto oneSecondNS = std::chrono::duration_cast<std::chrono::nanoseconds>(oneSecond);
   gSystemInfo->clockSpeed = oneSecondNS.count() * 4;

   // Set startup time
   gSystemInfo->baseTime = OSGetTime();
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
   RegisterKernelFunction(OSGetTitleID);
   RegisterKernelFunction(OSGetOSID);
}

namespace coreinit
{

namespace internal
{

void
setTitleID(uint64_t id)
{
   gTitleID = id;
}

void
setSystemID(uint64_t id)
{
   gSystemID = id;
}

} // namespace internal

} // namespace coreinit
