#pragma once
#include "modules/coreinit/coreinit_fs.h"

using TempStatus = FSStatus;

TempStatus
TEMPInit();

void
TEMPShutdown();
