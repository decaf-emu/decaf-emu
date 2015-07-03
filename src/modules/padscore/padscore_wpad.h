#pragma once
#include "systemtypes.h"

enum class WPADStatus
{
   Uninitialised = 0,
   Initialised = 1
};

void
WPADInit();

WPADStatus
WPADGetStatus();
