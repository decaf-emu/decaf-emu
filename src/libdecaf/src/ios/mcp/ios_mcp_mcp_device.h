#pragma once
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_response.h"
#include "ios_mcp_mcp_request.h"

namespace ios::mcp::internal
{

MCPError
mcpGetSysProdSettings(phys_ptr<MCPResponseGetSysProdSettings> response);

MCPError
mcpGetTitleId(phys_ptr<kernel::ResourceRequest> resourceRequest,
              phys_ptr<MCPResponseGetTitleId> response);

MCPError
mcpLoadFile(phys_ptr<MCPRequestLoadFile> request,
            phys_ptr<void> outputBuffer,
            uint32_t outputBufferLength);

MCPError
mcpPrepareTitle52(phys_ptr<MCPRequestPrepareTitle> request,
                  phys_ptr<MCPResponsePrepareTitle> response);

MCPError
mcpSwitchTitle(phys_ptr<MCPRequestSwitchTitle> request);

} // namespace ios::mcp::internal
