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
   decaf_warn_stub();
}

WPADError::Value
WPADProbe(WPADChan::Chan chan,
          be_val<WPADControllerType::Value> *type)
{
   if (type) {
      *type = WPADControllerType::NoController;
   }

   decaf_warn_stub();
   return WPADError::NoController;
}

uint32_t
WPADGetBatteryLevel(WPADChan::Chan chan)
{
   // Battery level is 0 - 4
   decaf_warn_stub();
   return 4;
}

int8_t
WPADGetSpeakerVolume()
{
   decaf_warn_stub();
   return 0;
}

void
WPADControlMotor(WPADChan::Chan chan,
                 BOOL enabled)
{
   decaf_warn_stub();
}

void
WPADDisconnect(WPADChan::Chan chan)
{
   decaf_warn_stub();
}

void
WPADEnableWiiRemote(BOOL enable)
{
   decaf_warn_stub();
}

void
Module::registerWPADFunctions()
{
   RegisterKernelFunction(WPADInit);
   RegisterKernelFunction(WPADGetStatus);
   RegisterKernelFunction(WPADEnableURCC);
   RegisterKernelFunction(WPADProbe);
   RegisterKernelFunction(WPADGetBatteryLevel);
   RegisterKernelFunction(WPADGetSpeakerVolume);
   RegisterKernelFunction(WPADControlMotor);
   RegisterKernelFunction(WPADDisconnect);
   RegisterKernelFunction(WPADEnableWiiRemote);
}

} // namespace padscore
