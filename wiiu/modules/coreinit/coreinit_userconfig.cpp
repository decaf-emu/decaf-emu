#include "coreinit.h"
#include "coreinit_userconfig.h"

IOHandle
UCOpen()
{
   return IOInvalidHandle;
}

void
UCClose(IOHandle handle)
{
}

IOError
UCReadSysConfig(IOHandle handle, uint32_t count, UCSysConfig *settings)
{
   return IOError::Generic;
}

void
CoreInit::registerUserConfigFunctions()
{
   RegisterSystemFunction(UCOpen);
   RegisterSystemFunction(UCClose);
   RegisterSystemFunction(UCReadSysConfig);
}
