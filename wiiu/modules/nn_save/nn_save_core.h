#pragma once
#include "systemtypes.h"

enum class SaveError : int32_t
{
   OK = 0,
   Error = -0x400
};

SaveError
SAVEInit();

void
SAVEShutdown();
