#pragma once
#include "ios/kernel/ios_kernel_process.h"

namespace ios::crypto
{

Error
processEntryPoint(phys_ptr<void> context);

} // namespace ios::crypto
