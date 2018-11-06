#include "coreinit.h"
#include "coreinit_cosreport.h"
#include "coreinit_driver.h"
#include "coreinit_dynload.h"
#include "coreinit_log.h"
#include "coreinit_memory.h"
#include "coreinit_osreport.h"
#include "coreinit_spinlock.h"
#include "coreinit_systemheap.h"
#include "coreinit_systeminfo.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_stackobject.h"

#include <common/decaf_assert.h>
#include <common/strutils.h>
#include <libcpu/cpu.h>

namespace cafe::coreinit
{

struct StaticDriverData
{
   be2_val<uint32_t> minUnk0x10;
   be2_val<uint32_t> maxUnk0x10;
   be2_struct<OSSpinLock> lock;
   be2_val<BOOL> didInit;
   be2_val<uint32_t> numRegistered;
   be2_virt_ptr<OSDriver> registeredDrivers;
   be2_virt_ptr<OSDriver> actionCallbackDriver;
};

static virt_ptr<StaticDriverData>
sDriverData = nullptr;

static int32_t
unkRegisterSyscall0x3200(virt_ptr<const char> name,
                         uint32_t nameLen,
                         virt_ptr<uint32_t> outUnk1,
                         virt_ptr<uint32_t> outUnk2)
{
   // TODO: Syscall 0x3200
   return 0;
}

static int32_t
unkDeregisterSyscall0x3300(virt_ptr<const char> name,
                           uint32_t nameLen)
{
   // TODO: Syscall 0x3300
   return 0;
}


/**
 * Register a driver.
 *
 * \param moduleHandle
 * The module handle to associate this driver with.
 *
 * \param unk1
 * Unknown, maybe something like a priority?
 *
 * \param driverInterface
 * Implementation of the driver interface functions.
 *
 * \param userDriverId
 * A user provided unique id to identify this driver.
 *
 * \param[out] outUnk1
 * Unknown, returned from registered driver with the kernel.
 *
 * \param[out] outUnk2
 * Unknown, returned from registered driver with the kernel.
 *
 * \param[out] outDidInit
 * Set to 1 after internal:driverInitialise has been called, 0 before.
 *
 * \return
 * Returns OSDriver_Error::OK on success, an error code otherwise.
 */
OSDriver_Error
OSDriver_Register(OSDynLoad_ModuleHandle moduleHandle,
                  uint32_t unk1, // Something like a priority
                  virt_ptr<OSDriverInterface> driverInterface,
                  OSDriver_UserDriverId userDriverId,
                  virt_ptr<uint32_t> outUnk1,
                  virt_ptr<uint32_t> outUnk2,
                  virt_ptr<BOOL> outDidInit)
{
   StackObject<OSDynLoad_ModuleHandle> callerModule;
   auto error = OSDriver_Error::OK;

   if (outUnk1) {
      *outUnk1 = 0u;
   }

   if (outUnk2) {
      *outUnk2 = 0u;
   }

   if (outDidInit) {
      *outDidInit = sDriverData->didInit;
   }

   if (!driverInterface || !driverInterface->getName) {
      return OSDriver_Error::InvalidArgument;
   }

   // Get the drivers name through the driverInterface
   auto name = cafe::invoke(cpu::this_core::state(),
                            driverInterface->getName,
                            userDriverId);
   if (!name) {
      return OSDriver_Error::InvalidArgument;
   }

   auto nameLen = static_cast<uint32_t>(strlen(name.get()));
   if (nameLen == 0 || nameLen >= 64) {
      return OSDriver_Error::InvalidArgument;
   }

   // Get the module handle for the caller of this function
   auto dynloadError =
      OSDynLoad_AcquireContainingModule(
         virt_cast<void *>(virt_addr { cpu::this_core::state()->lr }),
         OSDynLoad_SectionType::CodeOnly,
         callerModule);
   if (dynloadError != OSDynLoad_Error::OK) {
      internal::OSPanic("OSDrivers.c", 288,
                        "***OSDriver_Register could not find caller module.\n");
   }

   if (!moduleHandle) {
      moduleHandle = *callerModule;
   }

   // Allocate memory for the driver structure
   auto driver = virt_cast<OSDriver *>(OSAllocFromSystem(sizeof(OSDriver), 4));
   if (internal::isAppDebugLevelNotice()) {
      internal::COSInfo(
         COSReportModule::Unknown2,
         fmt::format(
            "RPL_SYSHEAP:DRIVER_REG,ALLOC,=\"{}\",-{}",
            driver, sizeof(OSDriver)));
   }

   if (!driver) {
      if (internal::isAppDebugLevelNotice()) {
         internal::dumpSystemHeap();
      }

      OSDynLoad_Release(*callerModule);
      return OSDriver_Error::OutOfSysMemory;
   }

   memset(driver, 0, sizeof(OSDriver));

   // Get module handle for driverInterface.getName
   dynloadError =
      OSDynLoad_AcquireContainingModule(
         virt_cast<void *>(driverInterface->getName.getAddress()),
         OSDynLoad_SectionType::CodeOnly,
         virt_addrof(driver->interfaceModuleHandles[0]));
   if (dynloadError != OSDynLoad_Error::OK) {
      internal::COSWarn(
         COSReportModule::Unknown1,
         fmt::format(
            "*** OSDriver_Register - failed to acquire containing module for driver \"{}\" Name() @ {}",
            name, driverInterface->getName));
      error = static_cast<OSDriver_Error>(dynloadError);
      goto error;
   }

   // Get module handle for driverInterface.onInit
   if (driverInterface->onInit) {
      dynloadError =
         OSDynLoad_AcquireContainingModule(
            virt_cast<void *>(driverInterface->onInit.getAddress()),
            OSDynLoad_SectionType::CodeOnly,
            virt_addrof(driver->interfaceModuleHandles[1]));
      if (dynloadError != OSDynLoad_Error::OK) {
         internal::COSWarn(
            COSReportModule::Unknown1,
            fmt::format(
               "*** OSDriver_Register - failed to acquire containing module for driver \"{}\" AutoInit() @ {}",
               name, driverInterface->onInit));
         error = static_cast<OSDriver_Error>(dynloadError);
         goto error;
      }
   }

   // Get module handle for driverInterface.onAcquiredForeground
   if (driverInterface->onAcquiredForeground) {
      dynloadError =
         OSDynLoad_AcquireContainingModule(
            virt_cast<void *>(driverInterface->onAcquiredForeground.getAddress()),
            OSDynLoad_SectionType::CodeOnly,
            virt_addrof(driver->interfaceModuleHandles[2]));
      if (dynloadError != OSDynLoad_Error::OK) {
         internal::COSWarn(
            COSReportModule::Unknown1,
            fmt::format(
               "*** OSDriver_Register - failed to acquire containing module for driver \"{}\" OnAcquiredForeground() @ {}",
               name, driverInterface->onAcquiredForeground));
         error = static_cast<OSDriver_Error>(dynloadError);
         goto error;
      }
   }

   // Get module handle for driverInterface.onReleasedForeground
   if (driverInterface->onReleasedForeground) {
      dynloadError =
         OSDynLoad_AcquireContainingModule(
            virt_cast<void *>(driverInterface->onReleasedForeground.getAddress()),
            OSDynLoad_SectionType::CodeOnly,
            virt_addrof(driver->interfaceModuleHandles[3]));
      if (dynloadError != OSDynLoad_Error::OK) {
         internal::COSWarn(
            COSReportModule::Unknown1,
            fmt::format(
               "*** OSDriver_Register - failed to acquire containing module for driver \"{}\" OnReleasedForeground() @ {}",
               name, driverInterface->onReleasedForeground));
         error = static_cast<OSDriver_Error>(dynloadError);
         goto error;
      }
   }

   // Get module handle for driverInterface.onDone
   if (driverInterface->onDone) {
      dynloadError =
         OSDynLoad_AcquireContainingModule(
            virt_cast<void *>(driverInterface->onDone.getAddress()),
            OSDynLoad_SectionType::CodeOnly,
            virt_addrof(driver->interfaceModuleHandles[4]));
      if (dynloadError != OSDynLoad_Error::OK) {
         internal::COSWarn(
            COSReportModule::Unknown1,
            fmt::format(
               "*** OSDriver_Register - failed to acquire containing module for driver \"{}\" AutoDone() @ {}",
               name, driverInterface->onDone));
         error = static_cast<OSDriver_Error>(dynloadError);
         goto error;
      }
   }

   OSUninterruptibleSpinLock_Acquire(virt_addrof(sDriverData->lock));

   // Check if a driver has already been registered with the same name
   for (auto other = sDriverData->registeredDrivers; other; other = other->next) {
      auto otherName = cafe::invoke(cpu::this_core::state(),
                                    other->interfaceFunctions.getName,
                                    other->userDriverId);
      if (iequals(otherName.get(), name.get())) {
         error = OSDriver_Error::AlreadyRegistered;
         OSUninterruptibleSpinLock_Release(virt_addrof(sDriverData->lock));
         goto error;
      }
   }

   dynloadError = static_cast<OSDynLoad_Error>(
      unkRegisterSyscall0x3200(name,
                               nameLen,
                               virt_addrof(driver->unk0x40),
                               virt_addrof(driver->unk0x44)));
   OSLogPrintf(0, 1, 0, "%s: Reg=%s, ms=%d", "OSDriver_Register", name, 0);
   if (dynloadError == OSDynLoad_Error::OK) {
      // Initialise driver data
      if (sDriverData->didInit) {
         driver->unk0x08 = 1u;
         driver->inForeground = OSGetForegroundBucket(nullptr, nullptr);
      }

      driver->coreID = cpu::this_core::id();
      driver->moduleHandle = moduleHandle;
      driver->interfaceFunctions = *driverInterface;
      driver->userDriverId = userDriverId;
      driver->unk0x10 = unk1;

      if (driver->unk0x10 < sDriverData->minUnk0x10) {
         sDriverData->minUnk0x10 = driver->unk0x10;
      }

      if (driver->unk0x10 > sDriverData->maxUnk0x10) {
         sDriverData->maxUnk0x10 = driver->unk0x10;
      }

      // Insert to front of list
      driver->next = sDriverData->registeredDrivers;
      sDriverData->registeredDrivers = driver;

      if (outUnk1) {
         *outUnk1 = driver->unk0x40;
      }

      if (outUnk2) {
         *outUnk2 = driver->unk0x44;
      }

      sDriverData->numRegistered++;
   }
   OSUninterruptibleSpinLock_Release(virt_addrof(sDriverData->lock));

   // Deduplicate dynload allocated handles
   for (auto i = 0u; i < driver->interfaceModuleHandles.size(); ++i) {
      if (driver->interfaceModuleHandles[i] == moduleHandle) {
         OSDynLoad_Release(driver->interfaceModuleHandles[i]);
         driver->interfaceModuleHandles[i] = 0u;
      }
   }

   OSDynLoad_Release(*callerModule);
   return OSDriver_Error::OK;

error:
   for (auto i = 0u; i < driver->interfaceModuleHandles.size(); ++i) {
      if (driver->interfaceModuleHandles[i]) {
         OSDynLoad_Release(driver->interfaceModuleHandles[i]);
      }
   }

   OSDynLoad_Release(*callerModule);

   if (internal::isAppDebugLevelNotice()) {
      internal::COSInfo(
         COSReportModule::Unknown2,
         fmt::format(
            "RPL_SYSHEAP:DRIVER_REG, FREE, =\"{}\",{}",
            driver, sizeof(OSDriver)));
   }
   OSFreeToSystem(driver);
   return error;
}


/**
 * Deregister a driver.
 *
 * \param moduleHandle
 * Module handle the driver was registered under.
 *
 * \param userDriverId
 * The user's driver id for the driver to deregister.
 *
 * \return
 * Returns OSDriver_Error::OK on success, an error code otherwise.
 */
OSDriver_Error
OSDriver_Deregister(OSDynLoad_ModuleHandle moduleHandle,
                    OSDriver_UserDriverId userDriverId)
{
   if (!sDriverData->numRegistered) {
      return OSDriver_Error::DriverNotFound;
   }

   OSUninterruptibleSpinLock_Acquire(virt_addrof(sDriverData->lock));

   // Find the driver in the registered driver linked list
   auto prev = virt_ptr<OSDriver> { nullptr };
   auto driver = sDriverData->registeredDrivers;
   while (driver) {
      if (driver->moduleHandle == moduleHandle &&
          driver->userDriverId == userDriverId) {
         break;
      }

      prev = driver;
      driver = driver->next;
   }

   if (!driver) {
      OSUninterruptibleSpinLock_Release(virt_addrof(sDriverData->lock));
      return OSDriver_Error::DriverNotFound;
   }

   // Check if we are trying to deregister from "inside action callback"
   if (sDriverData->actionCallbackDriver == driver) {
      internal::COSWarn(COSReportModule::Unknown1,
                        "***OSDriver_Deregister() of self from inside action callback!\n");
      sDriverData->actionCallbackDriver = nullptr;
   }

   // Remove the driver from the linked list
   if (prev) {
      prev->next = driver->next;
   } else {
      sDriverData->registeredDrivers = driver->next;
   }

   // Grab the drivers name for kernel deregister
   auto name = cafe::invoke(cpu::this_core::state(),
                            driver->interfaceFunctions.getName,
                            userDriverId);

   OSUninterruptibleSpinLock_Release(virt_addrof(sDriverData->lock));

   auto startTicks = OSGetTick();

   // Deregister driver with the kernel
   if (driver->unk0x40 == driver->unk0x44) {
      unkDeregisterSyscall0x3300(name,
                                 static_cast<uint32_t>(strlen(name.get())));
   }

   // Free the dynload handles
   for (auto handle : driver->interfaceModuleHandles) {
      if (handle) {
         OSDynLoad_Release(handle);
      }
   }

   if (internal::isAppDebugLevelNotice()) {
      internal::COSInfo(
         COSReportModule::Unknown2,
         fmt::format(
            "RPL_SYSHEAP:DRIVER_REG, FREE, =\"{}\",{}\n",
            driver, sizeof(OSDriver)));
   }

   // Free the driver
   OSFreeToSystem(driver);

   OSLogPrintf(0, 1, 0, "%s: Reg=%s, ms=%d", "OSDriver_Deregister",
               name,
               internal::ticksToMs(OSGetTick() - startTicks));
   return OSDriver_Error::OK;
}

OSDriver_Error
OSDriver_CopyFromSaveArea(OSDriver_UserDriverId driverId,
                          virt_ptr<void> data,
                          uint32_t size)
{
   return OSDriver_Error::DriverNotFound;
}

OSDriver_Error
OSDriver_CopyToSaveArea(OSDriver_UserDriverId driverId,
                        const virt_ptr<void> data,
                        uint32_t size)
{
   return OSDriver_Error::DriverNotFound;
}

namespace internal
{

void
driverOnInit()
{
   sDriverData->didInit = TRUE;

   if (!sDriverData->numRegistered) {
      return;
   }

   // TODO: Driver init should be called from driver action thread
   for (auto driver = sDriverData->registeredDrivers; driver; driver = driver->next) {
      cafe::invoke(cpu::this_core::state(),
                   driver->interfaceFunctions.onInit,
                   driver->userDriverId);
   }
}

void
driverOnDone()
{
   // TODO: OSDriver OnDone
}

} // namespace internal

void
Library::registerDriverSymbols()
{
   RegisterFunctionExport(OSDriver_Register);
   RegisterFunctionExport(OSDriver_Deregister);
   RegisterFunctionExport(OSDriver_CopyFromSaveArea);
   RegisterFunctionExport(OSDriver_CopyToSaveArea);

   RegisterDataInternal(sDriverData);
}

} // namespace cafe::coreinit
