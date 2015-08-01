#pragma once
#include <chrono>
#include "systemtypes.h"

extern std::chrono::time_point<std::chrono::system_clock>
gEpochTime;

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

std::chrono::time_point<std::chrono::system_clock>
OSTimeToChrono(OSTime time);
