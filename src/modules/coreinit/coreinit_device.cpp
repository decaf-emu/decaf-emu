#include "coreinit.h"
#include "coreinit_device.h"

static BOOL
OSDriver_Register(uint32_t r3, uint32_t r4, void *somePtr, uint32_t someID, uint32_t r7, uint32_t *someOutValue, uint32_t r9)
{
   return FALSE;
}

void
OSEnforceInorderIO()
{
}

uint16_t
OSReadRegister16(uint32_t device, uint32_t id)
{
   return 0;
}

uint32_t
OSReadRegister32Ex(uint32_t device, uint32_t id)
{
   return 0;
}

void
OSWriteRegister32Ex(uint32_t device, uint32_t id, uint32_t value)
{
}

void
CoreInit::registerDeviceFunctions()
{
   RegisterKernelFunction(OSDriver_Register);
   RegisterKernelFunction(OSReadRegister16);
   RegisterKernelFunction(OSEnforceInorderIO);
   RegisterKernelFunctionName("__OSReadRegister32Ex", OSReadRegister32Ex);
   RegisterKernelFunctionName("__OSWriteRegister32Ex", OSWriteRegister32Ex);
}
