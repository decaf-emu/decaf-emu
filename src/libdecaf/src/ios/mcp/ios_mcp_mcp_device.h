#pragma once
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_response.h"

namespace ios::mcp::internal
{

MCPError
mcpGetSysProdSettings(phys_ptr<MCPResponseGetSysProdSettings> response);

MCPError
mcpGetTitleId(phys_ptr<kernel::ResourceRequest> resourceRequest,
              phys_ptr<MCPResponseGetTitleId> response);

} // namespace ios::mcp::internal
