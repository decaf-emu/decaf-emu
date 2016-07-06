#include "coreinit.h"
#include "coreinit_im.h"

namespace coreinit
{

static BOOL
sAPDEnabled = TRUE;

static BOOL
sDimEnabled = TRUE;

BOOL
IMDisableAPD()
{
   sAPDEnabled = FALSE;
}

BOOL
IMDisableDim()
{
   sDimEnabled = FALSE;
}

BOOL
IMEnableAPD()
{
   sAPDEnabled = TRUE;
}

BOOL
IMEnableDim()
{
   sDimEnabled = TRUE;
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
