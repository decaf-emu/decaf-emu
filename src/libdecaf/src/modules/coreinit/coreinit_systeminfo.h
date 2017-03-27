#pragma once
#include "coreinit_enum.h"
#include "coreinit_time.h"

#include <common/be_val.h>
#include <common/cbool.h>
#include <common/structsize.h>
#include <cstdint>

namespace coreinit
{

#pragma pack(push, 1)

struct OSSystemInfo
{
   be_val<uint32_t> busSpeed;
   be_val<uint32_t> coreSpeed;
   be_val<OSTime> baseTime;
   be_val<uint32_t> _unkX[3];
   be_val<uint32_t> _unkY;
};
CHECK_OFFSET(OSSystemInfo, 0x0, busSpeed);
CHECK_OFFSET(OSSystemInfo, 0x4, coreSpeed);
CHECK_OFFSET(OSSystemInfo, 0x8, baseTime);
CHECK_OFFSET(OSSystemInfo, 0x10, _unkX);
CHECK_OFFSET(OSSystemInfo, 0x1c, _unkY);
CHECK_SIZE(OSSystemInfo, 0x20);

#pragma pack(pop)

int
bspGetHardwareVersion(be_val<HardwareVersion> *version);

OSSystemInfo *
OSGetSystemInfo();

BOOL
OSSetScreenCapturePermission(BOOL enabled);

BOOL
OSGetScreenCapturePermission();

uint32_t
OSGetConsoleType();

uint32_t
OSGetSecurityLevel();

BOOL
OSEnableHomeButtonMenu(BOOL enable);

BOOL
OSIsHomeButtonMenuEnabled();

void
OSBlockThreadsOnExit();

uint64_t
OSGetTitleID();

uint64_t
OSGetOSID();

OSShutdownReason
OSGetShutdownReason();

} // namespace coreinit
