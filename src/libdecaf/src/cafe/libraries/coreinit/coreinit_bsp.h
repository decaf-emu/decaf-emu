#pragma once
#include "coreinit_enum.h"
#include "ios/bsp/ios_bsp_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

using BSPHardwareVersion = ios::bsp::HardwareVersion;

BSPError
bspInitializeShimInterface();

BSPError
bspGetHardwareVersion(virt_ptr<BSPHardwareVersion> version);

namespace internal
{

BSPError
bspShutdownShimInterface();

} // namespace internal

} // namespace cafe::coreinit
