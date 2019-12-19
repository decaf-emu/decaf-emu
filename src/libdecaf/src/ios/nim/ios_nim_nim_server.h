#pragma once
#include "ios/ios_error.h"

namespace ios::nim::internal
{

Error
startNimServer();

Error
joinNimServer();

void
initialiseStaticNimServerData();

} // namespace ios::nim::internal
