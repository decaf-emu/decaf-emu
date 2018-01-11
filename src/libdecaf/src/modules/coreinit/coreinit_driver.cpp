#include "coreinit.h"
#include "coreinit_driver.h"

namespace coreinit
{

/*
OSDriver_Register(-1, 0x28, ptr, 0x0) FS
OSDriver_Register(0, 0x7D0, ptr, 0xCAFE0005) Button
OSDriver_Register(0, 0xA, ptr, 0xCAFE0004) CACHE
OSDriver_Register(0, 100000, ptr, 0x64) TEST
OSDriver_Register(0, 900, ptr, 0x14) ACPLoad
OSDriver_Register(0, 800, ptr, 0x9) Input
OSDriver_Register(0, 1000, ptr, 0xA) Clipboard
OSDriver_Register(0, 30, ptr, 0x1E) OSSetting
OSDriver_Register(0, -1, ptr, 0xCAFE0002) MEM
OSDriver_Register(0, 20, ptr, 0xCAFE0001) IPC
OSDriver_Register(rpl_entry.r3, 910, ptr, 0x0) ACP
OSDriver_Register(rpl_entry.r3, 150, ptr, 0x6) AVM
OSDriver_Register(rpl_entry.r3, 550, ptr, 0x0) CAM
OSDriver_Register(rpl_entry.r3, 100, ptr, 0x7) DC
OSDriver_Register(rpl_entry.r3, 250, ptr, 0) GX2
OSDriver_Register(rpl_entry.r3, 550, ptr, 0x10000814) ccr_mic
OSDriver_Register(rpl_entry.r3, 50, ptr, 0) NETWORK
OSDriver_Register(rpl_entry.r3, 50, ptr, 0) NETWORK
OSDriver_Register(rpl_entry.r3, 50, ptr, 0) NETWORK
OSDriver_Register(rpl_entry.r3, 500, ptr, 0xC) HPAD
OSDriver_Register(rpl_entry.r3, 50, ptr, 0) NETWORK2
OSDriver_Register(rpl_entry.r3, 950, ptr, 0) TEMP
OSDriver_Register(rpl_entry.r3, 50, ptr, 0xB) CCR
OSDriver_Register(rpl_entry.r3, 450, ptr, 0) HID
OSDriver_Register(rpl_entry.r3, 50, ptr, 0) NETWORK
OSDriver_Register(rpl_entry.r3, 160, ptr, 0) UHS
OSDriver_Register(rpl_entry.r3, 0x12C, ptr, 0) NSYSUVD
OSDriver_Register(rpl_entry.r3, 0x226, ptr, 0) NTAG
OSDriver_Register(rpl_entry.r3, 150, ptr, 2) WBC
OSDriver_Register(rpl_entry.r3, 150, ptr, 3) KPAD
OSDriver_Register(rpl_entry.r3, 50, ptr, 2) WPAD
OSDriver_Register(rpl_entry.r3, 0xC8, ptr, 0) ProcUI
...
*/

struct OSRegisteredDriver
{
   // +0x00 = module Handle (r3 of OSDriver_Register)
   // +0x04 = id (r6 of OSDriver_Register)
   // +0x08 = 1
   // +0x0C = return value of OSGetForegroundBucket
   // +0x10 = r4 of OSDriver_Register
   // +0x14 = core id during OSDriver_Register
   UNKNOWN(0x2C); // 0x00
   be2_val<uint32_t> module_Name;
   be2_val<uint32_t> module_AutoInit;
   be2_val<uint32_t> module_OnAcquiredForeground;
   be2_val<uint32_t> module_OnReleasedForeground;
   be2_val<uint32_t> module_AutoDone;
   // +0x40 r5 to __OSDrivers_SYSCALL_GetInstance
   // +0x44 r6 to __OSDrivers_SYSCALL_GetInstance
   // +0x48 = *10047CD0 = prevDriver
   // 10047CD0 is linked list of driver, points to last registered
   UNKNOWN(0xC); // 0x40
};
CHECK_SIZE(OSRegisteredDriver, 0x4C);

BOOL
OSDriver_Register(ModuleHandle moduleHandle,
                  uint32_t inUnk1, // priority or a time?
                  OSDriverInterface *driverInterface,
                  uint32_t id,
                  uint32_t *outUnk1,
                  uint32_t *outUnk2,
                  uint32_t *outUnk3)
{
   // TODO: OSDriver_Register
   return FALSE;
}

void
Module::registerDriverFunctions()
{
   RegisterKernelFunction(OSDriver_Register);
}

} // namespace coreinit
