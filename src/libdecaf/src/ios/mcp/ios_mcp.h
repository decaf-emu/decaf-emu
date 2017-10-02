#pragma once
#include "ios_mcp_enum.h"
#include "ios/kernel/ios_kernel_process.h"

#include <cstdint>

namespace ios::mcp
{

Error
processEntryPoint(phys_ptr<void> context);

namespace internal
{

uint32_t
getBootFlags();

uint32_t
getSystemModeFlags();

SystemFileSys
getSystemFileSys();

} // namespace internal

} // namespace ios::mcp
