#include "coreinit.h"
#include "coreinit_device.h"

uint16_t
OSReadRegister16(uint32_t device, uint32_t id)
{
   return 0;
}

void
CoreInit::registerDeviceFunctions()
{
   RegisterSystemFunction(OSReadRegister16);
}