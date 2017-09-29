#pragma once
#include "ios/ios_enum.h"

namespace ios::auxil::internal
{

Error
startImThread();

Error
stopImThread();

void
initialiseStaticImThreadData();

} // namespace ios::auxil::internal