#pragma once
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_types.h"

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

struct MCPResponseGetTitleId
{
   be2_val<uint64_t> titleId;
};
CHECK_OFFSET(MCPResponseGetTitleId, 0x00, titleId);
CHECK_SIZE(MCPResponseGetTitleId, 8);

struct MCPResponseGetOwnTitleInfo
{
   be2_struct<MCPTitleListType> titleInfo;
};
CHECK_OFFSET(MCPResponseGetOwnTitleInfo, 0x00, titleInfo);
CHECK_SIZE(MCPResponseGetOwnTitleInfo, 0x61);

struct MCPResponseGetSysProdSettings
{
   be2_struct<MCPSysProdSettings> settings;
};
CHECK_OFFSET(MCPResponseGetSysProdSettings, 0x00, settings);
CHECK_SIZE(MCPResponseGetSysProdSettings, 0x46);

#pragma pack(pop)

/** @} */

} // namespace ios::mcp
