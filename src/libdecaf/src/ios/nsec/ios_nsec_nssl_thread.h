#pragma once
#include "ios/ios_enum.h"

namespace ios::nsec::internal
{

Error
registerNsslResourceManager();

Error
startNsslThread();

Error
stopNsslThread();

void
initialiseStaticNsslData();

} // namespace ios::nsec::internal
