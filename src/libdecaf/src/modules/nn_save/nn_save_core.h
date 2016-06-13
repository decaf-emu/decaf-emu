#pragma once
#include "modules/coreinit/coreinit_fs.h"
#include "common/types.h"

using SaveStatus = FSStatus;

namespace nn
{

namespace save
{

SaveStatus
SAVEInit();

void
SAVEShutdown();

} // namespace save

} // namespace nn
