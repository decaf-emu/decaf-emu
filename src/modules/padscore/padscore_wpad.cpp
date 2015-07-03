#include "padscore.h"
#include "padscore_wpad.h"

static WPADStatus
gWPADStatus = WPADStatus::Uninitialised;

void
WPADInit()
{
   gWPADStatus = WPADStatus::Initialised;
}

WPADStatus
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
