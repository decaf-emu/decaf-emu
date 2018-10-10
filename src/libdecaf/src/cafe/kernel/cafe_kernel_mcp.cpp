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
mcpGetFileLength(std::string_view path,
                 uint32_t *outSize,
                 MCPFileType fileType,
                 uint32_t a4)
{
   auto buffer = ipcAllocBuffer(sizeof(MCPRequestGetFileLength));

   // Prepare request
   auto request = virt_cast<MCPRequestGetFileLength *>(buffer);
   request->fileType = fileType;
   request->unk0x18 = a4;
   request->name = path;

   // Send ioctl
   auto error = IOS_Ioctl(RamPartitionId::Kernel,
                          RamPartitionId::Invalid,
                          getMcpHandle(),
                          MCPCommand::GetFileLength,
                          buffer, sizeof(MCPRequestGetFileLength),
                          nullptr, 0u);
   if (error >= ios::Error::OK) {
      *outSize = static_cast<uint32_t>(error);
   }

   ipcFreeBuffer(buffer);
   return error;
}

ios::Error
mcpLoadFile(std::string_view path,
            virt_ptr<void> dataBuffer,
            uint32_t size,
            uint32_t pos,
            MCPFileType fileType,
            UniqueProcessId cafeProcessId)
{
   auto buffer = ipcAllocBuffer(sizeof(MCPRequestLoadFile));

   // Prepare request
   auto request = virt_cast<MCPRequestLoadFile *>(buffer);
   request->fileType = fileType;
   request->name = path;
   request->pos = pos;
   request->cafeProcessId = static_cast<uint32_t>(cafeProcessId);

   // Send ioctl
   auto error = IOS_Ioctl(RamPartitionId::Kernel,
                          RamPartitionId::Invalid,
                          getMcpHandle(),
                          MCPCommand::LoadFile,
                          buffer, sizeof(MCPRequestLoadFile),
                          dataBuffer, size);

   ipcFreeBuffer(buffer);
   return error;
}

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
      std::memcpy(outTitleInfo.get(),
                  virt_addrof(response->titleInfo).get(),
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
