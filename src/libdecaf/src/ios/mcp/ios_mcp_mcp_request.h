#pragma once
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_types.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace ios::mcp
{

/**
 * \ingroup ios_dev_mcp
 * @{
 */

#pragma pack(push, 1)

struct MCPRequestGetOwnTitleInfo
{
   be2_val<uint32_t> unk0x00;
};
CHECK_OFFSET(MCPRequestGetOwnTitleInfo, 0x00, unk0x00);
CHECK_SIZE(MCPRequestGetOwnTitleInfo, 0x04);

struct MCPRequestSearchTitleList
{
   be2_struct<MCPTitleListType> searchTitle;
   be2_val<MCPTitleListSearchFlags> searchFlags;
};
CHECK_OFFSET(MCPRequestSearchTitleList, 0x00, searchTitle);
CHECK_OFFSET(MCPRequestSearchTitleList, 0x61, searchFlags);
CHECK_SIZE(MCPRequestSearchTitleList, 0x65);

#pragma pack(pop)

/** @} */

} // namespace ios::mcp
