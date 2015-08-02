#include "coreinit.h"
#include "coreinit_shared.h"

BOOL
OSGetSharedData(OSSharedDataType::Type type, uint32_t, be_val<uint32_t> *addr, be_val<uint32_t> *size)
{
   // TODO: OSGetSharedData
   return FALSE;
}

void
CoreInit::registerSharedFunctions()
{
   RegisterKernelFunction(OSGetSharedData);
}

