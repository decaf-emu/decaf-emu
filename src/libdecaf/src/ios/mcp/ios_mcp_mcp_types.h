#pragma once
#include "ios_mcp_config.h"
#include "ios_mcp_enum.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace ios::mcp
{

/**
 * \ingroup ios_mcp
 * @{
 */

#pragma pack(push, 1)

using MCPSysProdSettings = internal::SysProdConfig;

struct MCPTitleListType
{
   be2_val<uint64_t> titleId;
   UNKNOWN(4);
   be2_array<char, 56> path;
   be2_val<MCPAppType> appType;
   UNKNOWN(0x54 - 0x48);
   be2_val<uint8_t> device;
   UNKNOWN(1);
   be2_array<char, 10> indexedDevice;
   be2_val<uint8_t> unk0x60;
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

} // namespace ios::mcp
