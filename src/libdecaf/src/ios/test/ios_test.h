#pragma once
#include "ios/kernel/ios_kernel_process.h"

namespace ios::test
{

Error
processEntryPoint(phys_ptr<void> context);

} // namespace ios::test
