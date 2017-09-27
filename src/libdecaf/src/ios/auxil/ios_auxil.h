#pragma once
#include "ios/ios_enum.h"
#include <libcpu/be2_struct.h>

namespace ios::auxil
{

Error
processEntryPoint(phys_ptr<void> context);

} // namespace ios::auxil
