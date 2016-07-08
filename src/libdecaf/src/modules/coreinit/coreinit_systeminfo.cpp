#include <chrono>
#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "coreinit_memheap.h"
#include "coreinit_time.h"
#include "common/platform_time.h"

namespace coreinit
{

enum class HardwareVersion : uint32_t
{
   UNKNOWN = 0x00000000,

   // vWii Hardware Versions
   HOLLYWOOD_ENG_SAMPLE_1 = 0x00000001,
   HOLLYWOOD_ENG_SAMPLE_2 = 0x10000001,
   HOLLYWOOD_PROD_FOR_WII = 0x10100001,
   HOLLYWOOD_CORTADO = 0x10100008,
   HOLLYWOOD_CORTADO_ESPRESSO = 0x1010000C,
   BOLLYWOOD = 0x20000001,
   BOLLYWOOD_PROD_FOR_WII = 0x20100001,

   // WiiU Hardware Versions
   LATTE_A11_EV = 0x21100010,
   LATTE_A11_CAT = 0x21100020,
   LATTE_A12_EV = 0x21200010,
   LATTE_A12_CAT = 0x21200020,

   LATTE_A2X_EV = 0x22100010,
   LATTE_A2X_CAT = 0x22100020,

   LATTE_A3X_EV = 0x23100010,
   LATTE_A3X_CAT = 0x23100020,
   LATTE_A3X_CAFE = 0x23100028,

   LATTE_A4X_EV = 0x24100010,
   LATTE_A4X_CAT = 0x24100020,
   LATTE_A4X_CAFE = 0x24100028,

   LATTE_A5X_EV = 0x25100010,
   LATTE_A5X_EV_Y = 0x25100011,
   LATTE_A5X_CAT = 0x25100020,
   LATTE_A5X_CAFE = 0x25100028,

   LATTE_B1X_EV = 0x26100010,
   LATTE_B1X_EV_Y = 0x26100011,
   LATTE_B1X_CAT = 0x26100020,
   LATTE_B1X_CAFE = 0x26100028,
};

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

HardwareVersion
bspGetHardwareVersion()
{
   return HardwareVersion::LATTE_B1X_CAFE;
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
   RegisterKernelFunction(bspGetHardwareVersion);
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
