#pragma once
#include "ios/ios_enum.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"

namespace ios::mcp
{

using MCPHandle = kernel::ResourceHandleId;

Error
MCP_Open();

Error
MCP_Close(MCPHandle handle);

Error
MCP_RegisterResourceManager(std::string_view device,
                            kernel::MessageQueueId messageQueueId);

} // namespace ios::mcp
