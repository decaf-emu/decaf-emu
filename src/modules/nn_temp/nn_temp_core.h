#pragma once
#include "modules/coreinit/coreinit_fs.h"

using TempStatus = FSStatus;
using TempDirID = uint64_t;

TempStatus
TEMPInit();

void
TEMPShutdown();
