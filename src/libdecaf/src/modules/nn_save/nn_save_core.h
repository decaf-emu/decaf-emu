#pragma once
#include "modules/coreinit/coreinit_fs.h"
#include "common/types.h"

namespace nn
{

namespace save
{

using SaveStatus = coreinit::FSStatus;

SaveStatus
SAVEInit();

void
SAVEShutdown();

} // namespace save

} // namespace nn
