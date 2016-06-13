#pragma once
#include "modules/coreinit/coreinit_fs.h"

namespace nn
{

namespace temp
{

using TempStatus = coreinit::FSStatus;
using TempDirID = uint64_t;

TempStatus
TEMPInit();

void
TEMPShutdown();

} // namespace temp

} // namespace nn

