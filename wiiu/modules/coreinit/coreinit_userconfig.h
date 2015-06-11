#pragma once
#include "systemtypes.h"
#include "coreinit_ios.h"

#pragma pack(push, 1)

struct UCSysConfig
{
   UNKNOWN(0x54); // From UCReadSysConfigAsync, memcpy count * 0x54? Not sure if correct
};

#pragma pack(pop)

IOHandle
UCOpen();

void
UCClose(IOHandle handle);

IOError
UCReadSysConfig(IOHandle handle, uint32_t count, UCSysConfig *settings);
