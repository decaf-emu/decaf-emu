#pragma once
#include "cafe/libraries/coreinit/coreinit_driver.h"

namespace cafe::nn_acp::internal
{

void
startDriver(cafe::coreinit::OSDynLoad_ModuleHandle moduleHandle);

void
stopDriver(cafe::coreinit::OSDynLoad_ModuleHandle moduleHandle);

} // namespace cafe::nn_acp::internal
