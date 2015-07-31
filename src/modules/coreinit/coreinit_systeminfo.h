#pragma once
#include <chrono>
#include "systemtypes.h"
#include "coreinit_time.h"

#pragma pack(push, 1)

struct OSSystemInfo
{
   be_val<uint32_t> clockSpeed;
   UNKNOWN(0x4);
   be_val<OSTime> baseTime;
   UNKNOWN(0x10);
};
CHECK_OFFSET(OSSystemInfo, 0x0, clockSpeed);
CHECK_OFFSET(OSSystemInfo, 0x8, baseTime);
CHECK_SIZE(OSSystemInfo, 0x20);

#pragma pack(pop)

extern std::chrono::time_point<std::chrono::system_clock>
gEpochTime;

OSSystemInfo *
OSGetSystemInfo();

BOOL
OSSetScreenCapturePermission(BOOL enabled);

BOOL
OSGetScreenCapturePermission();

uint32_t
OSGetConsoleType();
