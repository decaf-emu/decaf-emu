#pragma once
#include "systemtypes.h"
#include "modules/coreinit/coreinit_fs.h"

using TempStatus = FSStatus;

TempStatus
TEMPInit();

void
TEMPShutdown();
