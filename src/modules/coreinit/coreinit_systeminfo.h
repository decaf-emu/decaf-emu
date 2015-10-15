#pragma once
#include "be_val.h"
#include "coreinit_time.h"
#include "structsize.h"

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

OSSystemInfo *
OSGetSystemInfo();

BOOL
OSSetScreenCapturePermission(BOOL enabled);

BOOL
OSGetScreenCapturePermission();

uint32_t
OSGetConsoleType();

BOOL
OSEnableHomeButtonMenu(BOOL enable);

void
OSBlockThreadsOnExit();
