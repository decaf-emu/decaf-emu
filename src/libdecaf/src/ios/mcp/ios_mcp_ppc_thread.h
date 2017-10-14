#pragma once
#include "ios/ios_enum.h"

namespace ios::mcp::internal
{

Error
startPpcThread();

void
initialiseStaticPpcThreadData();

} // namespace ios::mcp::internal
