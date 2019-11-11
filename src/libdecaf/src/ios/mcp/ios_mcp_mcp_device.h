#pragma once
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_response.h"
#include "ios_mcp_mcp_request.h"

namespace ios::mcp::internal
{

MCPError
mcpDeviceList(phys_ptr<const MCPRequestDeviceList> request,
              phys_ptr<MCPDevice> deviceList,
              uint32_t deviceListSizeBytes);

MCPError
mcpGetFileLength(phys_ptr<const MCPRequestGetFileLength> request);

MCPError
mcpGetSysProdSettings(phys_ptr<MCPResponseGetSysProdSettings> response);

MCPError
mcpGetTitleId(phys_ptr<kernel::ResourceRequest> resourceRequest,
              phys_ptr<MCPResponseGetTitleId> response);

MCPError
mcpLoadFile(phys_ptr<const MCPRequestLoadFile> request,
            phys_ptr<void> outputBuffer,
            uint32_t outputBufferLength);

MCPError
mcpPrepareTitle52(phys_ptr<const MCPRequestPrepareTitle> request,
                  phys_ptr<MCPResponsePrepareTitle> response);

MCPError
mcpSwitchTitle(phys_ptr<const MCPRequestSwitchTitle> request);

MCPError
mcpUpdateCheckContext(phys_ptr<MCPResponseUpdateCheckContext> response);

MCPError
mcpUpdateCheckResume(phys_ptr<MCPResponseUpdateCheckResume> response);

MCPError
mcpUpdateGetProgress(phys_ptr<MCPResponseUpdateProgress> response);

} // namespace ios::mcp::internal
