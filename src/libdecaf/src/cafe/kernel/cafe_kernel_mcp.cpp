#include "cafe_kernel_ipc.h"
#include "cafe_kernel_mcp.h"
#include "cafe_kernel_processid.h"

#include "ios/mcp/ios_mcp_enum.h"
#include "ios/mcp/ios_mcp_mcp_types.h"
#include "ios/mcp/ios_mcp_mcp_request.h"
#include "ios/mcp/ios_mcp_mcp_response.h"

namespace cafe::kernel::internal
{

using namespace ios::mcp;

ios::Error
mcpPrepareTitle(MCPTitleId titleId,
                virt_ptr<MCPPPrepareTitleInfo> outTitleInfo)
{
   auto buffer = ipcAllocBuffer(sizeof(MCPRequestPrepareTitle));

   // Prepare request
   auto request = virt_cast<MCPRequestPrepareTitle *>(buffer);
   request->titleId = titleId;

   // Send ioctl
   auto error = IOS_Ioctl(RamPartitionId::Kernel,
                          RamPartitionId::Invalid,
                          getMcpHandle(),
                          MCPCommand::PrepareTitle0x52,
                          buffer, sizeof(MCPRequestPrepareTitle),
                          buffer, sizeof(MCPResponsePrepareTitle));

   // Handle response
   if (error >= ios::Error::OK) {
      auto response = virt_cast<MCPResponsePrepareTitle *>(buffer);
      std::memcpy(outTitleInfo.getRawPointer(),
                  virt_addrof(response->titleInfo).getRawPointer(),
                  sizeof(MCPPPrepareTitleInfo));
   }

   ipcFreeBuffer(buffer);
   return error;
}

ios::Error
mcpSwitchTitle(RamPartitionId rampid,
               phys_addr dataStart,
               phys_addr codeGenStart,
               phys_addr codeEnd)
{
   auto buffer = ipcAllocBuffer(sizeof(MCPRequestSwitchTitle));

   // Prepare request
   auto request = virt_cast<MCPRequestSwitchTitle *>(buffer);
   request->cafeProcessId = static_cast<uint32_t>(rampid);
   request->dataStart = dataStart;
   request->codeGenStart = codeGenStart;
   request->codeEnd = codeEnd;

   // Send ioctl
   auto error = IOS_Ioctl(RamPartitionId::Kernel,
                          RamPartitionId::Invalid,
                          getMcpHandle(),
                          MCPCommand::SwitchTitle,
                          buffer, sizeof(MCPRequestSwitchTitle),
                          nullptr, 0u);

   ipcFreeBuffer(buffer);
   return error;
}

} // namespace cafe::kernel::internal
