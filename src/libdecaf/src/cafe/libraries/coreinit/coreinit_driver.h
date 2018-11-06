#pragma once
#include "coreinit_dynload.h"
#include "coreinit_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

#pragma pack(push, 1)

using OSDriver_UserDriverId = uint32_t;

using OSDriver_GetNameFn = virt_func_ptr<virt_ptr<const char>(OSDriver_UserDriverId)>;
using OSDriver_OnInitFn = virt_func_ptr<void(OSDriver_UserDriverId)>;
using OSDriver_OnAcquiredForegroundFn = virt_func_ptr<void(OSDriver_UserDriverId)>;
using OSDriver_OnReleasedForegroundFn = virt_func_ptr<void(OSDriver_UserDriverId)>;
using OSDriver_OnDoneFn = virt_func_ptr<void(OSDriver_UserDriverId)>;

struct OSDriverInterface
{
   //! Return the driver name
   be2_val<OSDriver_GetNameFn> getName;

   //! Called to initialise the driver.
   be2_val<OSDriver_OnInitFn> onInit;

   //! Called when application is brought to foreground.
   be2_val<OSDriver_OnAcquiredForegroundFn> onAcquiredForeground;

   //! Called when application is sent to background.
   be2_val<OSDriver_OnReleasedForegroundFn> onReleasedForeground;

   //! Called when driver is done.
   be2_val<OSDriver_OnDoneFn> onDone;
};

struct OSDriver
{
   //! Module handle of current RPL.
   be2_val<OSDynLoad_ModuleHandle> moduleHandle;

   //! First argument passed to all driver interface functions.
   be2_val<OSDriver_UserDriverId> userDriverId;

   //! Unknown, set to 1 in OSDriver_Register.
   be2_val<uint32_t> unk0x08;

   //! Whether OSDriver_Register was called when process is in foreground.
   be2_val<BOOL> inForeground;

   //! Unknown, value set from r4 of OSDriver_Register.
   be2_val<uint32_t> unk0x10;

   //!Core on which OSDriver_Register was called.
   be2_val<uint32_t> coreID;

   //! Interface function pointers.
   be2_struct<OSDriverInterface> interfaceFunctions;

   //! Module handles for each interface function.
   be2_array<OSDynLoad_ModuleHandle, 5> interfaceModuleHandles;

   //! Unknown, pointer to this passed as r5 to syscall 0x3200.
   be2_val<uint32_t> unk0x40;

   //! Unknown, pointer to this passed as r6 to syscall 0x3200.
   be2_val<uint32_t> unk0x44;

   //! Pointer to next OSDriver in linked list.
   be2_virt_ptr<OSDriver> next;
};
CHECK_OFFSET(OSDriver, 0x00, moduleHandle);
CHECK_OFFSET(OSDriver, 0x04, userDriverId);
CHECK_OFFSET(OSDriver, 0x08, unk0x08);
CHECK_OFFSET(OSDriver, 0x0C, inForeground);
CHECK_OFFSET(OSDriver, 0x10, unk0x10);
CHECK_OFFSET(OSDriver, 0x14, coreID);
CHECK_OFFSET(OSDriver, 0x18, interfaceFunctions);
CHECK_OFFSET(OSDriver, 0x2C, interfaceModuleHandles);
CHECK_OFFSET(OSDriver, 0x40, unk0x40);
CHECK_OFFSET(OSDriver, 0x44, unk0x44);
CHECK_OFFSET(OSDriver, 0x48, next);
CHECK_SIZE(OSDriver, 0x4C);

#pragma pack(pop)

OSDriver_Error
OSDriver_Register(OSDynLoad_ModuleHandle moduleHandle,
                  uint32_t unk1,
                  virt_ptr<OSDriverInterface> driverInterface,
                  OSDriver_UserDriverId userDriverId,
                  virt_ptr<uint32_t> outUnk1,
                  virt_ptr<uint32_t> outUnk2,
                  virt_ptr<BOOL> outDidOSDriverInit);

OSDriver_Error
OSDriver_Deregister(OSDynLoad_ModuleHandle moduleHandle,
                    OSDriver_UserDriverId userDriverId);

OSDriver_Error
OSDriver_CopyFromSaveArea(OSDriver_UserDriverId driverId,
                          virt_ptr<void> data,
                          uint32_t size);

OSDriver_Error
OSDriver_CopyToSaveArea(OSDriver_UserDriverId driverId,
                        const virt_ptr<void> data,
                        uint32_t size);

namespace internal
{

void
driverOnInit();

void
driverOnDone();

} // namespace internal

} // namespace cafe::coreinit
