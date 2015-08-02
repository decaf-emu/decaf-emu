#include "coreinit.h"
#include "coreinit_userconfig.h"

IOHandle
UCOpen()
{
   // TODO: UCOpen
   return IOInvalidHandle;
}

void
UCClose(IOHandle handle)
{
   // TODO: UCClose
}

IOError
UCReadSysConfig(IOHandle handle, uint32_t count, UCSysConfig *settings)
{
   // TODO: UCReadSysConfig
   return IOError::Generic;
}

void
CoreInit::registerUserConfigFunctions()
{
   RegisterKernelFunction(UCOpen);
   RegisterKernelFunction(UCClose);
   RegisterKernelFunction(UCReadSysConfig);
}
