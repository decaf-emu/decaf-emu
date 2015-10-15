#pragma once
#include "coreinit_ios.h"
#include "structsize.h"

#pragma pack(push, 1)

struct MCPSysProdSettings
{
   UNKNOWN(0x46); // From RedCarpet.rpx 0x2536508
};

#pragma pack(pop)

IOHandle
MCP_Open();

void
MCP_Close(IOHandle handle);

IOError
MCP_GetSysProdSettings(IOHandle handle, MCPSysProdSettings *settings);
