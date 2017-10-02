#pragma once
#include "ios_kernel_enum.h"
#include "ios/ios_enum.h"

namespace ios::kernel
{

Error
IOS_SetSecurityLevel(SecurityLevel level);

SecurityLevel
IOS_GetSecurityLevel();

namespace internal
{

void
setSecurityLevel(SecurityLevel level);

} // namespace internal

} // namespace ios::kernel
