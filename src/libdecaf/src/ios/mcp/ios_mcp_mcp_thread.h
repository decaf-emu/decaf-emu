#pragma once
#include "ios/ios_enum.h"

namespace ios::mcp::internal
{

Error
startMcpThread();

void
initialiseStaticMcpThreadData();

ios::Handle
getFsaHandle();

} // namespace ios::mcp::internal
