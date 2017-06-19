#pragma once
#include "mcp_enum.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/structsize.h>

namespace kernel
{

namespace ios
{

namespace mcp
{

/**
* \ingroup kernel_ios_mcp
* @{
*/

#pragma pack(push, 1)

struct MCPSysProdSettings
{
   UNKNOWN(3);
   be_val<MCPRegion> platformRegion;
   UNKNOWN(0x7);
   be_val<MCPRegion> gameRegion;
   UNKNOWN(0x3A);
};
CHECK_OFFSET(MCPSysProdSettings, 0x03, platformRegion);
CHECK_OFFSET(MCPSysProdSettings, 0x0B, gameRegion);
CHECK_SIZE(MCPSysProdSettings, 0x46);

struct MCPTitleListType
{
   be_val<uint64_t> titleId;
   UNKNOWN(4);
   char path[56];
   be_val<MCPAppType> appType;
   UNKNOWN(0x54 - 0x48);
   uint8_t device;
   UNKNOWN(1);
   char indexedDevice[10];
   uint8_t unk0x60;
};
CHECK_OFFSET(MCPTitleListType, 0x00, titleId);
CHECK_OFFSET(MCPTitleListType, 0x0C, path);
CHECK_OFFSET(MCPTitleListType, 0x44, appType);
CHECK_OFFSET(MCPTitleListType, 0x54, device);
CHECK_OFFSET(MCPTitleListType, 0x56, indexedDevice);
CHECK_OFFSET(MCPTitleListType, 0x60, unk0x60);
CHECK_SIZE(MCPTitleListType, 0x61);

#pragma pack(pop)

/** @} */

} // namespace mcp

} // namespace ios

} // namespace kernel
