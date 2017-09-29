#pragma once
#include "ios/kernel/ios_kernel_process.h"

namespace ios::pad
{

Error
processEntryPoint(phys_ptr<void> context);

} // namespace ios::pad
