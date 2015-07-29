#include "coreinit.h"
#include "coreinit_shared.h"

BOOL
OSGetSharedData(SharedType type, uint32_t, be_val<uint32_t> *addr, be_val<uint32_t> *size)
{
   return FALSE;
}

void
CoreInit::registerSharedFunctions()
{
   RegisterKernelFunction(OSGetSharedData);
}

