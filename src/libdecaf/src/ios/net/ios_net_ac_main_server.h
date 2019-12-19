#pragma once
#include "ios/ios_error.h"

namespace ios::net::internal
{

Error
startAcMainServer();

Error
joinAcMainServer();

void
initialiseStaticAcMainServerData();

} // namespace ios::net::internal
