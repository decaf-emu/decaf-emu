#pragma once
#include "cafe/libraries/coreinit/coreinit_driver.h"

namespace cafe::nn_acp::internal
{

void
startDriver(coreinit::OSDynLoad_ModuleHandle moduleHandle);

void
stopDriver(coreinit::OSDynLoad_ModuleHandle moduleHandle);

} // namespace cafe::nn_acp::internal
