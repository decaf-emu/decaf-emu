#pragma once
#include "cafe_kernel_processid.h"

#include "ios/ios_error.h"
#include "ios/mcp/ios_mcp_mcp_types.h"

#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::kernel::internal
{

ios::Error
mcpGetFileLength(std::string_view path,
                 uint32_t *outSize,
                 ios::mcp::MCPFileType fileType,
                 uint32_t a4);

ios::Error
mcpLoadFile(std::string_view path,
            virt_ptr<void> buffer,
            uint32_t size,
            uint32_t pos,
            ios::mcp::MCPFileType fileType,
            UniqueProcessId cafeProcessId);

ios::Error
mcpPrepareTitle(ios::mcp::MCPTitleId titleId,
                virt_ptr<ios::mcp::MCPPPrepareTitleInfo> outTitleInfo);

ios::Error
mcpSwitchTitle(RamPartitionId rampid,
               phys_addr dataStart,
               phys_addr codeGenStart,
               phys_addr physEnd);

} // namespace cafe::kernel::internal
