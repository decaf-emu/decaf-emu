#include "ios_mcp_config.h"
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_device.h"
#include "ios_mcp_mcp_types.h"
#include "ios_mcp_mcp_request.h"
#include "ios_mcp_mcp_response.h"

#include "ios/kernel/ios_kernel_process.h"

#include "ios/ios_stackobject.h"

namespace ios::mcp::internal
{

MCPError
mcpGetSysProdSettings(phys_ptr<MCPResponseGetSysProdSettings> response)
{
   std::memcpy(phys_addrof(response->settings).getRawPointer(),
               getSysProdConfig().getRawPointer(),
               sizeof(MCPSysProdSettings));
   return MCPError::OK;
}

MCPError
mcpGetTitleId(phys_ptr<kernel::ResourceRequest> resourceRequest,
              phys_ptr<MCPResponseGetTitleId> response)
{
   response->titleId = resourceRequest->requestData.titleId;
   return MCPError::OK;
}

} // namespace ios::mcp::internal
