#include "coreinit.h"
#include "coreinit_driver.h"

namespace coreinit
{

/*
OSDriver_Register(-1, 0x28, ptr, 0x0) FS
OSDriver_Register(0, 0x7D0, ptr, 0xCAFE0005) Button
OSDriver_Register(0, 0xA, ptr, 0xCAFE0004) CACHE
OSDriver_Register(0, 0x186A0, ptr, 0x64) TEST
OSDriver_Register(0, 0x384, ptr, 0x14) ACPLoad
OSDriver_Register(0, 0x320, ptr, 0x9) Input
OSDriver_Register(0, 0x3E8, ptr, 0xA) Clipboard
OSDriver_Register(0, 0x1E, ptr, 0x1E) OSSetting
OSDriver_Register(0, -1, ptr, 0xCAFE0002) MEM
OSDriver_Register(0, 0x14, ptr, 0xCAFE0001) IPC
OSDriver_Register(rpl_entry.r3, 0x38E, ptr, 0x0) ACP
OSDriver_Register(rpl_entry.r3, 0x96, ptr, 0x6) AVM
OSDriver_Register(rpl_entry.r3, 0x226, ptr, 0x0) CAM
OSDriver_Register(rpl_entry.r3, 0x64, ptr, 0x7) DC
OSDriver_Register(rpl_entry.r3, 0xFA, ptr, 0) GX2
OSDriver_Register(rpl_entry.r3, 0x226, ptr, 0x10000814) ccr_mic
OSDriver_Register(rpl_entry.r3, 0x32, ptr, 0) NETWORK
OSDriver_Register(rpl_entry.r3, 0x32, ptr, 0) NETWORK
OSDriver_Register(rpl_entry.r3, 0x32, ptr, 0) NETWORK
OSDriver_Register(rpl_entry.r3, 0x1F4, ptr, 0xC) HPAD
OSDriver_Register(rpl_entry.r3, 0x32, ptr, 0) NETWORK2
OSDriver_Register(rpl_entry.r3, 0x3B6, ptr, 0) TEMP
OSDriver_Register(rpl_entry.r3, 0x32, ptr, 0xB) CCR
OSDriver_Register(rpl_entry.r3, 0x1C2, ptr, 0) HID
OSDriver_Register(rpl_entry.r3, 0x32, ptr, 0) NETWORK
OSDriver_Register(rpl_entry.r3, 0xA0, ptr, 0) UHS
OSDriver_Register(rpl_entry.r3, 0x12C, ptr, 0) NSYSUVD
OSDriver_Register(rpl_entry.r3, 0x226, ptr, 0) NTAG
OSDriver_Register(rpl_entry.r3, 0x96, ptr, 2) WBC
OSDriver_Register(rpl_entry.r3, 0x96, ptr, 3) KPAD
OSDriver_Register(rpl_entry.r3, 0x32, ptr, 2) WPAD
OSDriver_Register(rpl_entry.r3, 0xC8, ptr, 0) ProcUI
...
*/

BOOL
OSDriver_Register(ModuleHandle moduleHandle,
                  uint32_t inUnk1,
                  OSDriverInterface driverInterface,
                  uint32_t inUnk2,
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
