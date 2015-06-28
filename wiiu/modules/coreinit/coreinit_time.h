#pragma once
#include "systemtypes.h"

using Ticks = int32_t;
using Time = int64_t;

// This is FILETIME to Seconds but * 4
static const Time
CLOCK_SPEED = 10000000 * 4;

Ticks
OSGetSystemTick();

Time
OSGetSystemTime();

Ticks
OSGetTick();

Time
OSGetTime();
