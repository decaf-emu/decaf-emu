#pragma once
#include "coreinit_enum.h"
#include "coreinit_ios.h"

#include <common/be_val.h>
#include <common/structsize.h>
#include <cstdint>

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
CHECK_OFFSET(MCPSysProdSettings, 0x0B, gameRegion);
CHECK_SIZE(MCPSysProdSettings, 0x46);

struct MCPTitleListType
{
   UNKNOWN(0x61);
};
CHECK_SIZE(MCPTitleListType, 0x61);

struct MCPResponseGetTitleId
{
   be_val<uint64_t> titleId;
};
CHECK_OFFSET(MCPResponseGetTitleId, 0x00, titleId);
CHECK_SIZE(MCPResponseGetTitleId, 8);

#pragma pack(pop)

IOSError
MCP_Open();

void
MCP_Close(IOSHandle handle);

MCPError
MCP_GetOwnTitleInfo(IOSHandle handle,
                    MCPTitleListType *titleInfo);

MCPError
MCP_GetSysProdSettings(IOSHandle handle,
                       MCPSysProdSettings *settings);

MCPError
MCP_GetTitleId(IOSHandle handle,
               be_val<uint64_t> *titleId);

namespace internal
{

void
mcpInit();

void *
mcpAllocateMessage(uint32_t size);

MCPError
mcpFreeMessage(void *message);

MCPError
mcpDecodeIosErrorToMcpError(IOSError error);

} // namespace internal

/** @} */

} // namespace coreinit
