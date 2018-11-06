#include "nn_acp.h"
#include "nn_acp_client.h"
#include "nn_acp_internal_driver.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_driver.h"
#include "cafe/libraries/coreinit/coreinit_cosreport.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"

using namespace cafe::coreinit;

namespace cafe::nn_acp::internal
{

struct StaticDriverData
{
   be2_val<BOOL> registered = FALSE;
   be2_val<BOOL> initialised = FALSE;
   be2_array<char, 16> name = "ACP";
   be2_struct<OSDriverInterface> interface;
};

static virt_ptr<StaticDriverData> sDriverData = nullptr;
static OSDriver_GetNameFn sDriverGetName = nullptr;
static OSDriver_OnInitFn sDriverOnInit = nullptr;
static OSDriver_OnAcquiredForegroundFn sDriverOnAcquiredForeground = nullptr;
static OSDriver_OnReleasedForegroundFn sDriverOnReleasedForeground = nullptr;
static OSDriver_OnDoneFn sDriverOnDone = nullptr;

static virt_ptr<const char>
getName(OSDriver_UserDriverId id)
{
   return virt_addrof(sDriverData->name);
}

static void
onInit(OSDriver_UserDriverId id)
{
   coreinit::internal::COSWarn(COSReportModule::Unknown1,
                               "   ACP_AutoInit: start\n");
   ACPInitialize();
   coreinit::internal::COSWarn(COSReportModule::Unknown1,
                               "   ACP_AutoInit: ACPInitialize complete\n");

   // TODO: ACPSaveDataInit()
   coreinit::internal::COSWarn(COSReportModule::Unknown1,
                               "   ACP_AutoInit: ACPSaveDataInit complete\n");

   // TODO: ACPNotifyPlayEvent(1)
   coreinit::internal::COSWarn(COSReportModule::Unknown1,
                               "   ACP_AutoInit: ACPNotifyPlayEvent complete\n");

   // TODO: NDMInitialize
   coreinit::internal::COSWarn(COSReportModule::Unknown1,
                               "   ACP_AutoInit: NDMInitialize complete\n");

   auto upid = OSGetUPID();
   if (upid == kernel::UniqueProcessId::Game ||
       upid == kernel::UniqueProcessId::HomeMenu) {
      // Check for bg daemon enable
   }

   sDriverData->initialised = TRUE;
}

static void
onAcquiredForeground(OSDriver_UserDriverId id)
{
}

static void
onReleasedForeground(OSDriver_UserDriverId id)
{
}

static void
onDone(OSDriver_UserDriverId id)
{
}

void
startDriver(OSDynLoad_ModuleHandle moduleHandle)
{
   if (sDriverData->registered) {
      return;
   }

   StackObject<BOOL> driversAlreadyInitialised;
   sDriverData->interface.getName = sDriverGetName;
   sDriverData->interface.onInit = sDriverOnInit;
   sDriverData->interface.onAcquiredForeground = sDriverOnAcquiredForeground;
   sDriverData->interface.onReleasedForeground = sDriverOnReleasedForeground;
   sDriverData->interface.onDone = sDriverOnDone;

   OSDriver_Register(moduleHandle, 910,
                     virt_addrof(sDriverData->interface),
                     0,
                     nullptr, nullptr,
                     driversAlreadyInitialised);

   if (*driversAlreadyInitialised) {
      onInit(0);
   }

   sDriverData->registered = TRUE;
}

void
stopDriver(OSDynLoad_ModuleHandle moduleHandle)
{
   if (sDriverData->registered) {
      OSDriver_Deregister(moduleHandle, 0);
      sDriverData->registered = FALSE;
   }
}

} // namespace cafe::nn_acp::internal

namespace cafe::nn_acp
{

void
Library::registerDriverSymbols()
{
   RegisterFunctionInternal(internal::getName,
                            internal::sDriverGetName);
   RegisterFunctionInternal(internal::onInit,
                            internal::sDriverOnInit);
   RegisterFunctionInternal(internal::onAcquiredForeground,
                            internal::sDriverOnAcquiredForeground);
   RegisterFunctionInternal(internal::onReleasedForeground,
                            internal::sDriverOnReleasedForeground);
   RegisterFunctionInternal(internal::onDone,
                            internal::sDriverOnDone);

   RegisterDataInternal(internal::sDriverData);
}

} // namespace cafe::nn_acp
