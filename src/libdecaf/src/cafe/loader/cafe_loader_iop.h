#pragma once
#include "cafe/kernel/cafe_kernel_processid.h"

#include "ios/ios_error.h"
#include "ios/ios_ipc.h"
#include "ios/mcp/ios_mcp_enum.h"

namespace cafe::loader::internal
{

void
LiInitIopInterface();

void
LiCheckAndHandleInterrupts();

ios::Error
LiLoadAsync(std::string_view name,
            virt_ptr<void> outBuffer,
            uint32_t outBufferSize,
            uint32_t pos,
            ios::mcp::MCPFileType fileType,
            kernel::RamPartitionId rampid);

ios::Error
LiWaitIopComplete(uint32_t *outBytesRead);

ios::Error
LiWaitIopCompleteWithInterrupts(uint32_t *outBytesRead);

void
initialiseIopStaticData();

} // namespace cafe::loader::internal
