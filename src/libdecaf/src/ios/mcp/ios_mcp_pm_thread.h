#pragma once
#include "ios/ios_enum.h"
#include "ios/kernel/ios_kernel_messagequeue.h"

#include <string_view>

namespace ios::mcp::internal
{

Error
startPmThread();

Error
registerResourceManager(std::string_view device,
                        kernel::MessageQueueId queue);

Error
handleResourceManagerRegistrations();

void
initialiseStaticPmThreadData();

} // namespace ios::mcp::internal
