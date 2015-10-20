#pragma once
#include "coreinit_ios.h"
#include "utils/structsize.h"

#pragma pack(push, 1)

struct UCSysConfig
{
   UNKNOWN(0x54); // From UCReadSysConfigAsync, memcpy count * 0x54? Not sure if correct
};
CHECK_SIZE(UCSysConfig, 0x54);

#pragma pack(pop)

IOHandle
UCOpen();

void
UCClose(IOHandle handle);

IOError
UCReadSysConfig(IOHandle handle, uint32_t count, UCSysConfig *settings);
