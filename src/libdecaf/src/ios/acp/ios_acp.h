#pragma once
#include "ios/ios_enum.h"
#include "ios/ios_ipc.h"
#include "ios/kernel/ios_kernel_process.h"

namespace ios::acp
{

Error
processEntryPoint(phys_ptr<void> context);

namespace internal
{

Handle
getFsaHandle();

} // namespace internal

} // namespace ios::acp
