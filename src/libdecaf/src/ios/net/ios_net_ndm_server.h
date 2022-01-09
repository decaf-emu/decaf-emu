#pragma once
#include "ios/ios_error.h"

namespace ios::net::internal
{

Error
startNdmServer();

Error
joinNdmServer();

void
initialiseStaticNdmServerData();

} // namespace ios::net::internal
