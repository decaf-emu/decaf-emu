#pragma once
#include "ios/ios_enum.h"

namespace ios::fs::internal
{

Error
startServiceThread();

void
initialiseStaticServiceThreadData();

} // namespace ios::fs::internal
