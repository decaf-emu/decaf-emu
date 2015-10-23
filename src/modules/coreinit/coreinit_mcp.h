#pragma once
#include "coreinit_ios.h"
#include "coreinit_sci.h"
#include "utils/be_val.h"
#include "utils/structsize.h"

#pragma pack(push, 1)

struct MCPSysProdSettings
{
   UNKNOWN(3);
   be_val<SCIRegion::Region> platformRegion;
   UNKNOWN(0x7);
   be_val<SCIRegion::Region> gameRegion;
   UNKNOWN(0x3A);
};
CHECK_OFFSET(MCPSysProdSettings, 0x03, platformRegion);
CHECK_OFFSET(MCPSysProdSettings, 0x0b, gameRegion);
CHECK_SIZE(MCPSysProdSettings, 0x46);

#pragma pack(pop)

IOHandle
MCP_Open();

void
MCP_Close(IOHandle handle);

IOError
MCP_GetSysProdSettings(IOHandle handle, MCPSysProdSettings *settings);
