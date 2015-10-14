#include "padscore.h"
#include "padscore_wpad.h"

static WPADStatus::Status
gWPADStatus = WPADStatus::Uninitialised;

void
WPADInit()
{
   gWPADStatus = WPADStatus::Initialised;
}

WPADStatus::Status
WPADGetStatus()
{
   return gWPADStatus;
}

void
PadScore::registerWPADFunctions()
{
   RegisterKernelFunction(WPADInit);
   RegisterKernelFunction(WPADGetStatus);
}
