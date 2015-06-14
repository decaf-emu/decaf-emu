#pragma once
#include "systemtypes.h"

using OSTime = int64_t;
using TimeTicks = int64_t;
using TimeClocks = int64_t;
using TimeNanoseconds = int64_t;

TimeClocks
OSGetSystemTime();
