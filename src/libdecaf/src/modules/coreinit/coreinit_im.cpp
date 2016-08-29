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

IOError
IMGetTimeBeforeAPD(be_val<uint32_t> *seconds)
{
   // Let's just put it to 1 hour
   *seconds = 60 * 60;
   return IOError::OK;
}

IOError
IMGetTimeBeforeDimming(be_val<uint32_t> *seconds)
{
   // Let's just put it to 30 minutes
   *seconds = 30 * 60;
   return IOError::OK;
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
   RegisterKernelFunction(IMGetTimeBeforeAPD);
   RegisterKernelFunction(IMGetTimeBeforeDimming);
}

} // namespace coreinit
