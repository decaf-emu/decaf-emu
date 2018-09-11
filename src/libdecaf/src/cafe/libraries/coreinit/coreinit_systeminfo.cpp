#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "coreinit_time.h"

#include "cafe/kernel/cafe_kernel_info.h"

#include <chrono>
#include <common/platform_time.h>

namespace cafe::coreinit
{

struct StaticSystemInfoData
{
   be2_struct<OSSystemInfo> systemInfo;
   be2_val<BOOL> screenCapturePermission;
   be2_val<BOOL> enableHomeButtonMenu;
   be2_struct<kernel::Info0> kernelInfo0;
   be2_struct<kernel::Info6> kernelInfo6;
   be2_array<char, 4096> argstr;
};

static virt_ptr<StaticSystemInfoData>
sSystemInfoData = nullptr;

virt_ptr<OSSystemInfo>
OSGetSystemInfo()
{
   return virt_addrof(sSystemInfoData->systemInfo);
}

BOOL
OSSetScreenCapturePermission(BOOL enabled)
{
   auto old = sSystemInfoData->screenCapturePermission;
   sSystemInfoData->screenCapturePermission = enabled;
   return old;
}

BOOL
OSGetScreenCapturePermission()
{
   return sSystemInfoData->screenCapturePermission;
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
   return sSystemInfoData->enableHomeButtonMenu;
}

BOOL
OSEnableHomeButtonMenu(BOOL enable)
{
   sSystemInfoData->enableHomeButtonMenu = enable;
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
   return sSystemInfoData->kernelInfo0.titleId;
}

uint64_t
OSGetOSID()
{
   return sSystemInfoData->kernelInfo6.osTitleId;
}

kernel::UniqueProcessId
OSGetUPID()
{
   return sSystemInfoData->kernelInfo0.upid;
}

OSAppFlags
OSGetAppFlags()
{
   return sSystemInfoData->kernelInfo0.appFlags;
}

OSShutdownReason
OSGetShutdownReason()
{
   return OSShutdownReason::NoShutdown;
}

void
OSGetArgcArgv(virt_ptr<uint32_t> argc,
              virt_ptr<virt_ptr<const char>> argv)
{
   *argc = 0u;
   *argv = nullptr;
}

BOOL
OSIsDebuggerPresent()
{
   return FALSE;
}

BOOL
OSIsDebuggerInitialized()
{
   return FALSE;
}

int32_t
ENVGetEnvironmentVariable(virt_ptr<const char> key,
                          virt_ptr<char> buffer,
                          uint32_t length)
{
   if (buffer) {
      *buffer = char { 0 };
   }

   return 0;
}

namespace internal
{

virt_ptr<char>
getArgStr()
{
   return virt_addrof(sSystemInfoData->argstr);
}

virt_ptr<void>
getCoreinitLoaderHandle()
{
   return sSystemInfoData->kernelInfo0.coreinit.loaderHandle;
}

virt_addr
getDefaultThreadStackBase(uint32_t coreId)
{
   if (coreId == 0) {
      return sSystemInfoData->kernelInfo0.stackBase0;
   } else if (coreId == 1) {
      return sSystemInfoData->kernelInfo0.stackBase1;
   } else if (coreId == 2) {
      return sSystemInfoData->kernelInfo0.stackBase2;
   } else {
      decaf_abort(fmt::format("Invalid coreId {}", coreId));
   }
}

virt_addr
getDefaultThreadStackEnd(uint32_t coreId)
{
   if (coreId == 0) {
      return sSystemInfoData->kernelInfo0.stackEnd0;
   } else if (coreId == 1) {
      return sSystemInfoData->kernelInfo0.stackEnd1;
   } else if (coreId == 2) {
      return sSystemInfoData->kernelInfo0.stackEnd2;
   } else {
      decaf_abort(fmt::format("Invalid coreId {}", coreId));
   }
}

virt_addr
getLockedCacheBaseAddress(uint32_t coreId)
{
   if (coreId == 0) {
      return sSystemInfoData->kernelInfo0.lockedCacheBase0;
   } else if (coreId == 1) {
      return sSystemInfoData->kernelInfo0.lockedCacheBase1;
   } else if (coreId == 2) {
      return sSystemInfoData->kernelInfo0.lockedCacheBase2;
   } else {
      decaf_abort(fmt::format("Invalid coreId {}", coreId));
   }
}

virt_addr
getMem2BaseAddress()
{
   return sSystemInfoData->kernelInfo0.dataAreaStart;
}

virt_addr
getMem2EndAddress()
{
   return sSystemInfoData->kernelInfo0.dataAreaEnd;
}

phys_addr
getMem2PhysAddress()
{
   return sSystemInfoData->kernelInfo0.physDataAreaStart;
}

virt_addr
getSdaBase()
{
   return sSystemInfoData->kernelInfo0.sdaBase;
}

virt_addr
getSda2Base()
{
   return sSystemInfoData->kernelInfo0.sda2Base;
}

uint32_t
getSystemHeapSize()
{
   return sSystemInfoData->kernelInfo0.systemHeapSize;
}

bool
isAppDebugLevelVerbose()
{
   return sSystemInfoData->kernelInfo0.appFlags.value()
      .debugLevel() >= OSAppFlagsDebugLevel::Verbose;
}

bool
isAppDebugLevelUnknown3()
{
   return sSystemInfoData->appFlags.value()
      .debugLevel() >= OSAppFlagsDebugLevel::Unknown3;
}

void
initialiseSystemInfo()
{
   sSystemInfoData->systemInfo.busSpeed = cpu::busClockSpeed;
   sSystemInfoData->systemInfo.coreSpeed = cpu::coreClockSpeed;
   sSystemInfoData->systemInfo.baseTime = internal::getBaseTime();
   sSystemInfoData->systemInfo.l2CacheSize[0] = 512 * 1024u;
   sSystemInfoData->systemInfo.l2CacheSize[1] = 2 * 1024 * 1024u;
   sSystemInfoData->systemInfo.l2CacheSize[2] = 512 * 1024u;
   sSystemInfoData->systemInfo.cpuRatio = 5u;

   sSystemInfoData->screenCapturePermission = TRUE;
   sSystemInfoData->enableHomeButtonMenu = TRUE;

   kernel::getInfo(kernel::InfoType::Type0,
                   virt_addrof(sSystemInfoData->kernelInfo0),
                   sizeof(kernel::Info0));

   kernel::getInfo(kernel::InfoType::Type6,
                   virt_addrof(sSystemInfoData->kernelInfo6),
                   sizeof(kernel::Info6));

   kernel::getInfo(kernel::InfoType::ArgStr,
                   virt_addrof(sSystemInfoData->argstr),
                   sSystemInfoData->argstr.size());
}

} // namespace internal

void
Library::registerSystemInfoSymbols()
{
   RegisterFunctionExport(OSGetSystemInfo);
   RegisterFunctionExport(OSSetScreenCapturePermission);
   RegisterFunctionExport(OSGetScreenCapturePermission);
   RegisterFunctionExport(OSGetConsoleType);
   RegisterFunctionExport(OSGetSecurityLevel);
   RegisterFunctionExport(OSEnableHomeButtonMenu);
   RegisterFunctionExport(OSIsHomeButtonMenuEnabled);
   RegisterFunctionExport(OSBlockThreadsOnExit);
   RegisterFunctionExport(OSGetTitleID);
   RegisterFunctionExport(OSGetOSID);
   RegisterFunctionExport(OSGetUPID);
   RegisterFunctionExport(OSGetShutdownReason);
   RegisterFunctionExport(OSGetArgcArgv);
   RegisterFunctionExport(OSIsDebuggerPresent);
   RegisterFunctionExport(OSIsDebuggerInitialized);
   RegisterFunctionExport(ENVGetEnvironmentVariable);
   RegisterFunctionExportName("_OSGetAppFlags", OSGetAppFlags);

   RegisterDataInternal(sSystemInfoData);
}

} // namespace cafe::coreinit
