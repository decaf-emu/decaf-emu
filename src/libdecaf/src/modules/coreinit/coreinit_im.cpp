#include "coreinit.h"
#include "coreinit_im.h"

namespace coreinit
{

static BOOL
sAPDEnabled = TRUE;

static BOOL
sDimEnabled = TRUE;

IOError
IMDisableAPD()
{
   sAPDEnabled = FALSE;
   return IOError::OK;
}

IOError
IMDisableDim()
{
   sDimEnabled = FALSE;
   return IOError::OK;
}

IOError
IMEnableAPD()
{
   sAPDEnabled = TRUE;
   return IOError::OK;
}

IOError
IMEnableDim()
{
   sDimEnabled = TRUE;
   return IOError::OK;
}

BOOL
IMIsAPDEnabled()
{
   return sAPDEnabled;
}

BOOL
IMIsAPDEnabledBySysSettings()
{
   return TRUE;
}

BOOL
IMIsDimEnabled()
{
   return sDimEnabled;
}

void
Module::registerImFunctions()
{
   RegisterKernelFunction(IMDisableAPD);
   RegisterKernelFunction(IMDisableDim);
   RegisterKernelFunction(IMEnableAPD);
   RegisterKernelFunction(IMEnableDim);
   RegisterKernelFunction(IMIsAPDEnabled);
   RegisterKernelFunction(IMIsAPDEnabledBySysSettings);
   RegisterKernelFunction(IMIsDimEnabled);
}

} // namespace coreinit
