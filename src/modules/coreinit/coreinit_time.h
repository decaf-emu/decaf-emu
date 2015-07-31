#pragma once
#include "systemtypes.h"

// Tick is 1 nanosecond
using OSTick = int32_t;

// Time is ticks since epoch
using OSTime = int64_t;

OSTime
OSGetTime();

OSTime
OSGetSystemTime();

OSTick
OSGetTick();

OSTick
OSGetSystemTick();
