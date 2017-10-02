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
handleResourceManagerRegistrations(uint32_t systemModeFlags,
                                   uint32_t bootFlags);

void
initialiseStaticPmThreadData();

} // namespace ios::mcp::internal
