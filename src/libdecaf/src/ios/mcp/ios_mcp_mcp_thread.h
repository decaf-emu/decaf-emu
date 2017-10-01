#pragma once
#include "ios/ios_enum.h"

namespace ios::mcp::internal
{

Error
startMcpThread();

void
initialiseStaticMcpThreadData();

} // namespace ios::mcp::internal
