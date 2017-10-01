#pragma once
#include "ios/ios_enum.h"
#include "ios/kernel/ios_kernel_messagequeue.h"

namespace ios::mcp::internal
{

Error
startPmThread();

Error
handleResourceManagerRegistrations();

void
initialiseStaticPmThreadData();

} // namespace ios::mcp::internal
