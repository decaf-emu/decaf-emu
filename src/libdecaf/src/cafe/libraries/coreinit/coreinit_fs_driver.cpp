#include "coreinit.h"
#include "coreinit_cosreport.h"
#include "coreinit_driver.h"

namespace cafe::coreinit
{

struct StaticFsDriverData
{
   be2_struct<OSDriverInterface> driverInterface;
   be2_array<char, 4> name;
   be2_val<BOOL> isDone;
};

virt_ptr<StaticFsDriverData>
sFsDriverData = nullptr;

static OSDriver_GetNameFn sFsDriverGetName;
static OSDriver_OnInitFn sFsDriverOnInit;
static OSDriver_OnAcquiredForegroundFn sFsDriverOnAcquiredForeground;
static OSDriver_OnReleasedForegroundFn sFsDriverOnReleasedForeground;
static OSDriver_OnDoneFn sFsDriverOnDone;

namespace internal
{

static virt_ptr<const char>
fsDriverGetName(OSDriver_UserDriverId id)
{
   return virt_addrof(sFsDriverData->name);
}

void
fsDriverOnInit(OSDriver_UserDriverId id)
{
}

void
fsDriverOnAcquiredForeground(OSDriver_UserDriverId id)
{
}

void
fsDriverOnReleasedForeground(OSDriver_UserDriverId id)
{
}

void
fsDriverOnDone(OSDriver_UserDriverId id)
{
   sFsDriverData->isDone = TRUE;
}

bool
fsDriverDone()
{
   return !!sFsDriverData->isDone;
}

void
initialiseFsDriver()
{
   sFsDriverData->name = "FS";
   sFsDriverData->isDone = FALSE;
   sFsDriverData->driverInterface.getName = sFsDriverGetName;
   sFsDriverData->driverInterface.onInit = sFsDriverOnInit;
   sFsDriverData->driverInterface.onAcquiredForeground = sFsDriverOnAcquiredForeground;
   sFsDriverData->driverInterface.onReleasedForeground = sFsDriverOnReleasedForeground;
   sFsDriverData->driverInterface.onDone = sFsDriverOnDone;

   auto driverError =
      OSDriver_Register(static_cast<OSDynLoad_ModuleHandle>(-1),
                        40,
                        virt_addrof(sFsDriverData->driverInterface),
                        0,
                        nullptr,
                        nullptr,
                        nullptr);

   COSVerbose(
      COSReportModule::Unknown5,
      fmt::format("FS: Registered to OSDriver: result {}", driverError));
}

} // namespace internal

void
Library::registerFsDriverSymbols()
{
   RegisterFunctionInternal(internal::fsDriverGetName, sFsDriverGetName);
   RegisterFunctionInternal(internal::fsDriverOnInit, sFsDriverOnInit);
   RegisterFunctionInternal(internal::fsDriverOnAcquiredForeground, sFsDriverOnAcquiredForeground);
   RegisterFunctionInternal(internal::fsDriverOnReleasedForeground, sFsDriverOnReleasedForeground);
   RegisterFunctionInternal(internal::fsDriverOnDone, sFsDriverOnDone);

   RegisterDataInternal(sFsDriverData);
}

} // namespace cafe::coreinit
