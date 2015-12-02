#pragma once
#include "modules/coreinit/coreinit_fs.h"

using TempStatus = FSStatus::Value;

TempStatus
TEMPInit();

void
TEMPShutdown();
