#pragma once
#include "ios/ios_enum.h"

namespace ios::fs::internal
{

Error
startFsaThread();

void
initialiseStaticFsaThreadData();

} // namespace ios::fs::internal
