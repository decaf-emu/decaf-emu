#pragma once
#include "systemtypes.h"

#pragma pack(push, 1)

struct OSSystemInfo
{
   be_val<uint32_t> clockSpeed;
   UNKNOWN(0x1c);
};
CHECK_OFFSET(OSSystemInfo, 0x0, clockSpeed);
CHECK_SIZE(OSSystemInfo, 0x20);

#pragma pack(pop)

p32<OSSystemInfo>
OSGetSystemInfo();

BOOL
OSSetScreenCapturePermission(BOOL enabled);

BOOL
OSGetScreenCapturePermission();

uint32_t
OSGetConsoleType();
