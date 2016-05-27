#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "coreinit_ios.h"
#include "utils/be_val.h"
#include "utils/structsize.h"

namespace coreinit
{

/**
 * \defgroup coreinit_mcp MCP
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct MCPSysProdSettings
{
   UNKNOWN(3);
   be_val<SCIRegion> platformRegion;
   UNKNOWN(0x7);
   be_val<SCIRegion> gameRegion;
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

/** @} */

} // namespace coreinit
