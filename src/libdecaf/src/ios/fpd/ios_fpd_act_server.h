#pragma once
#include "ios/ios_enum.h"
#include "ios/ios_ipc.h"

namespace ios::fpd::internal
{

Error
startActServer();

Handle
getActFsaHandle();

Handle
getActUserConfigHandle();

void
initialiseStaticActServerData();

} // namespace ios::fpd::internal
