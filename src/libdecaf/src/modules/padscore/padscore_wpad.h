#pragma once
#include "common/types.h"

namespace padscore
{

namespace WPADChan
{
enum Chan : uint32_t
{
   Chan0 = 0,
   Chan1 = 1,
   Chan2 = 2,
   Chan3 = 3
};
}

namespace WPADControllerType
{
enum Value : uint32_t
{
   Wiimote = 0,
   Nunchuk = 1,
   Classic = 2,
   NoController = 253,
};
}

namespace WPADMotorCommand
{
enum Command : uint32_t
{
   Stop = 0,
   Rumble = 1,
};
}

#define WPAD_MAX_CONTROLLERS_EXT 7

namespace WPADStatus
{
enum Status : uint32_t
{
   Uninitialised = 0,
   Initialised = 1
};
}

namespace WPADError
{
enum Value : int32_t
{
   None = 0,
   NoController = -1,
};
}

void
WPADInit();

WPADStatus::Status
WPADGetStatus();

void
WPADEnableURCC(BOOL enable);

WPADError::Value
WPADProbe(WPADChan::Chan chan,
          be_val<WPADControllerType::Value> *type);

uint32_t
WPADGetBatteryLevel(WPADChan::Chan chan);

} // namespace padscore
