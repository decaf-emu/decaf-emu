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

struct MCPRequestLoadFile
{
   UNKNOWN(0x10);
   be2_val<uint32_t> pos;
   be2_val<MCPFileType> fileType;
   be2_val<uint32_t> cafeProcessId;
   UNKNOWN(0xC);
   be2_array<char, 0x40> name;
   UNKNOWN(0x12D8 - 0x68);
};
CHECK_OFFSET(MCPRequestLoadFile, 0x10, pos);
CHECK_OFFSET(MCPRequestLoadFile, 0x14, fileType);
CHECK_OFFSET(MCPRequestLoadFile, 0x18, cafeProcessId);
CHECK_OFFSET(MCPRequestLoadFile, 0x28, name);
CHECK_SIZE(MCPRequestLoadFile, 0x12D8);

struct MCPRequestPrepareTitle
{
   be2_val<MCPTitleId> titleId;
   UNKNOWN(0x60);
   be2_array<char, 4096> argvStr;
   PADDING(0x12D8 - 0x1068);
};
CHECK_OFFSET(MCPRequestPrepareTitle, 0x00, titleId);
CHECK_OFFSET(MCPRequestPrepareTitle, 0x68, argvStr);
CHECK_SIZE(MCPRequestPrepareTitle, 0x12D8);

struct MCPRequestSearchTitleList
{
   be2_struct<MCPTitleListType> searchTitle;
   be2_val<MCPTitleListSearchFlags> searchFlags;
};
CHECK_OFFSET(MCPRequestSearchTitleList, 0x00, searchTitle);
CHECK_OFFSET(MCPRequestSearchTitleList, 0x61, searchFlags);
CHECK_SIZE(MCPRequestSearchTitleList, 0x65);

struct MCPRequestSwitchTitle
{
   UNKNOWN(0x18);
   be2_val<uint32_t> cafeProcessId;
   be2_val<phys_addr> dataStart;
   be2_val<phys_addr> codeEnd;
   be2_val<phys_addr> codeGenStart;
   PADDING(0x12D8 - 0x28);
};
CHECK_OFFSET(MCPRequestSwitchTitle, 0x18, cafeProcessId);
CHECK_OFFSET(MCPRequestSwitchTitle, 0x1C, dataStart);
CHECK_OFFSET(MCPRequestSwitchTitle, 0x20, codeEnd);
CHECK_OFFSET(MCPRequestSwitchTitle, 0x24, codeGenStart);
CHECK_SIZE(MCPRequestSwitchTitle, 0x12D8);

#pragma pack(pop)

/** @} */

} // namespace ios::mcp
