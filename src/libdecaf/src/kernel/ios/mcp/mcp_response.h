#pragma once
#include "mcp_enum.h"
#include "mcp_types.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
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

struct MCPResponseGetTitleId
{
   be_val<uint64_t> titleId;
};
CHECK_OFFSET(MCPResponseGetTitleId, 0x00, titleId);
CHECK_SIZE(MCPResponseGetTitleId, 8);

struct MCPResponseGetOwnTitleInfo
{
   MCPTitleListType titleInfo;
};
CHECK_OFFSET(MCPResponseGetOwnTitleInfo, 0x00, titleInfo);
CHECK_SIZE(MCPResponseGetOwnTitleInfo, 0x61);

#pragma pack(pop)

/** @} */

} // namespace mcp

} // namespace ios

} // namespace kernel
