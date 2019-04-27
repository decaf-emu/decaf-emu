#pragma once
#include "cpu.h"
#include "cpu_config.h"
#include "mem.h"

#include <array>
#include <condition_variable>
#include <memory>

namespace cpu
{

extern BranchTraceHandler gBranchTraceHandler;

Core *
getCore(int index);

SystemCallHandler
getSystemCallHandler(uint32_t id);

bool
initialiseMemory();

namespace this_core
{

void
updateRoundingMode();

} // namespace this_core

} // namespace cpu
