#pragma once
#include "systemtypes.h"
#include "modules/coreinit/coreinit_fs.h"

using SaveStatus = FSStatus;

SaveStatus
SAVEInit();

void
SAVEShutdown();

SaveStatus
SAVEInitSaveDir(uint8_t userID);
