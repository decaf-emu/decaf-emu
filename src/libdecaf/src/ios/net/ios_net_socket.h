#pragma once
#include "ios/ios_enum.h"

namespace ios::net::internal
{

Error
registerSocketResourceManager();

Error
startSocketThread();

Error
stopSocketThread();

void
initialiseStaticSocketData();

} // namespace ios::net::internal
