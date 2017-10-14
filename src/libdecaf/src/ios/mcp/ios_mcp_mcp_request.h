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

struct MCPRequestLoadLibraryChunk
{
   UNKNOWN(0x10);
   be2_val<uint32_t> pos;
   be2_val<MCPLibraryType> type;
   be2_val<uint32_t> cafeProcessId;
   UNKNOWN(0xC);
   be2_array<char, 0x40> name;
   UNKNOWN(0x12D8 - 0x68);
};
CHECK_OFFSET(MCPRequestLoadLibraryChunk, 0x10, pos);
CHECK_OFFSET(MCPRequestLoadLibraryChunk, 0x14, type);
CHECK_OFFSET(MCPRequestLoadLibraryChunk, 0x18, cafeProcessId);
CHECK_OFFSET(MCPRequestLoadLibraryChunk, 0x28, name);
CHECK_SIZE(MCPRequestLoadLibraryChunk, 0x12D8);

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
