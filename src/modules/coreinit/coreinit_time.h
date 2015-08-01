#pragma once
#include <chrono>
#include "systemtypes.h"

extern std::chrono::time_point<std::chrono::system_clock>
gEpochTime;

// Tick is 1 nanosecond
using OSTick = int32_t;

// Time is ticks since epoch
using OSTime = int64_t;

struct OSCalendarTime {
   // fields mostly match Posix's struct tm, so names are taken from that struct
   be_val<int32_t> tm_sec;			// 0
   be_val<int32_t> tm_min;			// 4
   be_val<int32_t> tm_hour;		// 8
   be_val<int32_t> tm_mday;		// 12
   be_val<int32_t> tm_mon;			// 16
   be_val<int32_t> tm_year;		// 20
};

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
