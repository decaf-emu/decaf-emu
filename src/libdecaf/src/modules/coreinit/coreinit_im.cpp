#include "coreinit.h"
#include "coreinit_im.h"

namespace coreinit
{

static BOOL
sAPDEnabled = TRUE;

static BOOL
sDimEnabled = TRUE;

IMError
IMDisableAPD()
{
   sAPDEnabled = FALSE;
   return IMError::OK;
}

IMError
IMDisableDim()
{
   sDimEnabled = FALSE;
   return IMError::OK;
}

IMError
IMEnableAPD()
{
   sAPDEnabled = TRUE;
   return IMError::OK;
}

IMError
IMEnableDim()
{
   sDimEnabled = TRUE;
   return IMError::OK;
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

IMError
IMGetTimeBeforeAPD(be_val<uint32_t> *seconds)
{
   // Let's just put it to 1 hour
   *seconds = 60 * 60;
   return IMError::OK;
}

IMError
IMGetTimeBeforeDimming(be_val<uint32_t> *seconds)
{
   // Let's just put it to 30 minutes
   *seconds = 30 * 60;
   return IMError::OK;
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
