#pragma once
#include "coreinit_mcp.h"

#include <common/be_val.h>
#include <common/structsize.h>
#include <cstdint>

namespace coreinit
{

#pragma pack(push, 1)

struct MCPRequestGetOwnTitleInfo
{
   be_val<uint32_t> unk0x00;
};
CHECK_OFFSET(MCPRequestGetOwnTitleInfo, 0x00, unk0x00);
CHECK_SIZE(MCPRequestGetOwnTitleInfo, 0x04);

struct MCPRequestSearchTitleList
{
   MCPTitleListType searchTitle;
   be_val<MCPTitleListSearchFlags> searchFlags;
};
CHECK_OFFSET(MCPRequestSearchTitleList, 0x00, searchTitle);
CHECK_OFFSET(MCPRequestSearchTitleList, 0x61, searchFlags);
CHECK_SIZE(MCPRequestSearchTitleList, 0x65);

#pragma pack(pop)

} // namespace coreinit
