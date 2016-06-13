#include <chrono>
#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "coreinit_memheap.h"
#include "coreinit_time.h"
#include "common/platform_time.h"

namespace coreinit
{

static OSSystemInfo *
sSystemInfo = nullptr;

static BOOL
sScreenCapturePermission = TRUE;

static BOOL
sEnableHomeButtonMenu = TRUE;

static uint64_t
sTitleID = 0;

static uint64_t
sSystemID = 0x000500101000400Aull;

OSSystemInfo *
OSGetSystemInfo()
{
   return sSystemInfo;
}

BOOL
OSSetScreenCapturePermission(BOOL enabled)
{
   auto old = sScreenCapturePermission;
   sScreenCapturePermission = enabled;
   return old;
}

BOOL
OSGetScreenCapturePermission()
{
   return sScreenCapturePermission;
}

uint32_t
OSGetConsoleType()
{
   // Value from a WiiU retail v4.0.0
   return 0x3000050;
}

BOOL
OSIsHomeButtonMenuEnabled()
{
   return sEnableHomeButtonMenu;
}

BOOL
OSEnableHomeButtonMenu(BOOL enable)
{
   sEnableHomeButtonMenu = enable;
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
   return sTitleID;
}

uint64_t
OSGetOSID()
{
   return sSystemID;
}

void
Module::initialiseSystemInformation()
{
   sSystemInfo = coreinit::internal::sysAlloc<OSSystemInfo>();

   // Clockspeed is 4 * 1 second in ticks
   auto oneSecond = std::chrono::seconds(1);
   auto oneSecondNS = std::chrono::duration_cast<std::chrono::nanoseconds>(oneSecond);
   sSystemInfo->clockSpeed = oneSecondNS.count() * 4;

   // Set startup time
   sSystemInfo->baseTime = OSGetTime();

   sScreenCapturePermission = TRUE;
   sEnableHomeButtonMenu = TRUE;
}

void
Module::registerSystemInfoFunctions()
{
   RegisterKernelFunction(OSGetSystemInfo);
   RegisterKernelFunction(OSSetScreenCapturePermission);
   RegisterKernelFunction(OSGetScreenCapturePermission);
   RegisterKernelFunction(OSGetConsoleType);
   RegisterKernelFunction(OSEnableHomeButtonMenu);
   RegisterKernelFunction(OSIsHomeButtonMenuEnabled);
   RegisterKernelFunction(OSBlockThreadsOnExit);
   RegisterKernelFunction(OSGetTitleID);
   RegisterKernelFunction(OSGetOSID);
}

namespace internal
{

void
setTitleID(uint64_t id)
{
   sTitleID = id;
}

void
setSystemID(uint64_t id)
{
   sSystemID = id;
}

} // namespace internal

} // namespace coreinit
