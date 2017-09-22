#pragma once
#include "ios/ios_enum.h"
#include <cstdint>

namespace ios::kernel
{

Error
IOS_GetCurrentProcessID();

Error
IOS_GetProcessName(ProcessID process,
                   char *buffer);

namespace internal
{

ProcessID
getCurrentProcessID();

} // namespace internal

} // namespace ios::kernel
