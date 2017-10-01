#pragma once
#include "ios/ios_enum.h"

namespace ios::net::internal
{

Error
initSubsys();

Error
startSubsys();

Error
stopSubsys();

void
initialiseStaticSubsysData();

} // namespace ios::net::internal
