#pragma once
#include "cafe_kernel_processid.h"

#include "ios/ios_error.h"
#include "ios/mcp/ios_mcp_mcp_types.h"

#include <libcpu/be2_struct.h>

namespace cafe::kernel::internal
{

ios::Error
mcpPrepareTitle(ios::mcp::MCPTitleId titleId,
                virt_ptr<ios::mcp::MCPPPrepareTitleInfo> outTitleInfo);

ios::Error
mcpSwitchTitle(RamPartitionId rampid,
               phys_addr dataStart,
               phys_addr codeGenStart,
               phys_addr physEnd);

} // namespace cafe::kernel::internal
