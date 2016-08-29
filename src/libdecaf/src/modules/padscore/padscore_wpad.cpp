#include "padscore.h"
#include "padscore_wpad.h"

namespace padscore
{

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

// Enable Wii U Pro Controllers (URCC)
void
WPADEnableURCC(BOOL enable)
{
}

WPADError::Value
WPADProbe(WPADChan::Chan chan,
          be_val<WPADControllerType::Value> *type)
{
   *type = WPADControllerType::NoController;
   return WPADError::NoController;
}

uint32_t
WPADGetBatteryLevel(WPADChan::Chan chan)
{
   // Battery level is 0 - 4
   return 4;
}

void
Module::registerWPADFunctions()
{
   RegisterKernelFunction(WPADInit);
   RegisterKernelFunction(WPADGetStatus);
   RegisterKernelFunction(WPADEnableURCC);
   RegisterKernelFunction(WPADProbe);
   RegisterKernelFunction(WPADGetBatteryLevel);
}

} // namespace padscore
