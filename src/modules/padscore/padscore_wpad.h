#pragma once
#include "types.h"

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

void
WPADInit();

WPADStatus::Status
WPADGetStatus();

void
WPADEnableURCC(BOOL enable);
