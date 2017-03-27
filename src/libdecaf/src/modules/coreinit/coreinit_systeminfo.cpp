#include <chrono>
#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "coreinit_memheap.h"
#include "coreinit_time.h"
#include <common/platform_time.h>
#include "kernel/kernel.h"

namespace coreinit
{

static OSSystemInfo *
sSystemInfo = nullptr;

static BOOL
sScreenCapturePermission = TRUE;

static BOOL
sEnableHomeButtonMenu = TRUE;

int
bspGetHardwareVersion(be_val<HardwareVersion> *version)
{
   *version = HardwareVersion::LATTE_B1X_CAFE;
   return 0;
}

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

uint32_t
OSGetSecurityLevel()
{
   return 0;
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
   return kernel::getGameInfo().app.title_id;
}

uint64_t
OSGetOSID()
{
   return kernel::getGameInfo().app.os_version;
}

OSShutdownReason
OSGetShutdownReason()
{
   return OSShutdownReason::NoShutdown;
}

void
Module::initialiseSystemInformation()
{
   sSystemInfo->busSpeed = cpu::busClockSpeed;
   sSystemInfo->coreSpeed = cpu::coreClockSpeed;
   sSystemInfo->baseTime = internal::getBaseTime();
   sSystemInfo->_unkX[0] = 0x80000;
   sSystemInfo->_unkX[1] = 0x200000;
   sSystemInfo->_unkX[2] = 0x80000;
   sSystemInfo->_unkY = 5;

   sScreenCapturePermission = TRUE;
   sEnableHomeButtonMenu = TRUE;
}

void
Module::registerSystemInfoFunctions()
{
   RegisterKernelFunction(bspGetHardwareVersion);
   RegisterKernelFunction(OSGetSystemInfo);
   RegisterKernelFunction(OSSetScreenCapturePermission);
   RegisterKernelFunction(OSGetScreenCapturePermission);
   RegisterKernelFunction(OSGetConsoleType);
   RegisterKernelFunction(OSGetSecurityLevel);
   RegisterKernelFunction(OSEnableHomeButtonMenu);
   RegisterKernelFunction(OSIsHomeButtonMenuEnabled);
   RegisterKernelFunction(OSBlockThreadsOnExit);
   RegisterKernelFunction(OSGetTitleID);
   RegisterKernelFunction(OSGetOSID);
   RegisterKernelFunction(OSGetShutdownReason);

   RegisterInternalData(sSystemInfo);
}

} // namespace coreinit
