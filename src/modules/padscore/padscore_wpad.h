#pragma once
#include "types.h"

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
