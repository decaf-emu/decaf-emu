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

struct MCPResponsePrepareTitle
{
   UNKNOWN(0x68);
   be2_struct<MCPPPrepareTitleInfo> titleInfo;
};
CHECK_OFFSET(MCPResponsePrepareTitle, 0x68, titleInfo);
CHECK_SIZE(MCPResponsePrepareTitle, 0x12D8);

struct MCPResponseUpdateProgress
{
   be2_struct<MCPUpdateProgress> progress;
};
CHECK_SIZE(MCPResponseUpdateProgress, 0x38);

struct MCPResponseUpdateCheckContext
{
   be2_val<uint32_t> result;
};
CHECK_OFFSET(MCPResponseUpdateCheckContext, 0x00, result);
CHECK_SIZE(MCPResponseUpdateCheckContext, 4);

struct MCPResponseUpdateCheckResume
{
   be2_val<uint32_t> result;
};
CHECK_OFFSET(MCPResponseUpdateCheckResume, 0x00, result);
CHECK_SIZE(MCPResponseUpdateCheckResume, 4);

#pragma pack(pop)

/** @} */

} // namespace ios::mcp
