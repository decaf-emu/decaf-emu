#pragma once
#include "coreinit_time.h"
#include "utils/be_val.h"
#include "utils/structsize.h"

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

uint64_t
OSGetTitleID();

uint64_t
OSGetOSID();

namespace coreinit
{

namespace internal
{

void
setTitleID(uint64_t id);

void
setSystemID(uint64_t id);

} // namespace internal

} // namespace coreinit
