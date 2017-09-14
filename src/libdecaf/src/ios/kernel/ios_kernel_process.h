#pragma once
#include "ios/ios_enum.h"
#include "ios/ios_result.h"
#include <cstdint>

namespace ios::kernel
{

Result<ProcessID>
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
