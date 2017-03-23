#pragma once
#include "coreinit_dynload.h"
#include "ppcutils/wfunc_ptr.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/cbool.h>
#include <common/structsize.h>

namespace coreinit
{

#pragma pack(push, 1)

using OSDriver_GetNameFn = wfunc_ptr<const char *>;
using OSDriver_OnInitFn = wfunc_ptr<void, uint32_t>;
using OSDriver_OnAcquiredForegroundFn = wfunc_ptr<void, uint32_t>;
using OSDriver_OnReleasedForegroundFn = wfunc_ptr<void, uint32_t>;
using OSDriver_OnDoneFn = wfunc_ptr<void, uint32_t>;

struct OSDriverInterface
{
   //! Return the driver name
   OSDriver_GetNameFn::be name;

   //! Called to initialise the driver.
   OSDriver_OnInitFn::be onInit;

   //! Called when application is brought to foreground.
   OSDriver_OnAcquiredForegroundFn::be onAcquiredForeground;

   //! Called when application is sent to background.
   OSDriver_OnReleasedForegroundFn::be onReleasedForeground;

   //! Called when driver is done.
   OSDriver_OnDoneFn::be onDone;
};

struct OSDriver
{
   //! Module handle of current RPL.
   be_ModuleHandle moduleHandle;

   //! Value set from r6 of OSDriver_Register.
   //! First argument passed to all driver interface functions.
   be_val<uint32_t> unk_r6_OSDriver_Register;

   //! Set to 1 in OSDriever_Register.
   be_val<uint32_t> unk0x08;

   //! Whether OSDriver_Register was called when process is in foreground.
   be_val<BOOL> inForeground;

   //! Value set from r4 of OSDriver_Register.
   be_val<uint32_t> unk_r4_OSDriver_Register;

   //!Core on which OSDriver_Register was called.
   be_val<uint32_t> coreID;

   //! Interface function pointers.
   OSDriverInterface interfaceFunctions;

   //! Module handles for each interface function.
   be_ModuleHandle interfaceModuleHandles[5];

   //! Pointer to this passed as r5 to syscall 0x3200.
   be_val<uint32_t> unk0x40;

   //! Pointer to this passed as r6 to syscall 0x3200.
   be_val<uint32_t> unk0x44;

   //! Pointer to next OSDriver in linked list.
   be_ptr<OSDriver> next;
};
CHECK_OFFSET(OSDriver, 0x00, moduleHandle);
CHECK_OFFSET(OSDriver, 0x04, unk_r6_OSDriver_Register);
CHECK_OFFSET(OSDriver, 0x08, unk0x08);
CHECK_OFFSET(OSDriver, 0x0C, inForeground);
CHECK_OFFSET(OSDriver, 0x10, unk_r4_OSDriver_Register);
CHECK_OFFSET(OSDriver, 0x14, coreID);
CHECK_OFFSET(OSDriver, 0x18, interfaceFunctions);
CHECK_OFFSET(OSDriver, 0x2C, interfaceModuleHandles);
CHECK_OFFSET(OSDriver, 0x40, unk0x40);
CHECK_OFFSET(OSDriver, 0x44, unk0x44);
CHECK_OFFSET(OSDriver, 0x48, next);
CHECK_SIZE(OSDriver, 0x4C);

#pragma pack(pop)

BOOL
OSDriver_Register(ModuleHandle moduleHandle,
                  uint32_t inUnk1,
                  OSDriverInterface *driverInterface,
                  uint32_t inUnk2,
                  uint32_t *outUnk1,
                  uint32_t *outUnk2,
                  uint32_t *outUnk3);

} // namespace coreinit
