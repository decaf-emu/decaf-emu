#pragma once
#include <cstdint>
#include "libcpu/state.h"

namespace debugger
{

bool
enabled();

bool
paused();

cpu::Core *
getPausedCoreState(uint32_t coreId);

uint32_t
getPauseInitiatorCoreId();

void
pauseAll();

void
resumeAll();

void
stepCoreInto(uint32_t coreId);

void
stepCoreOver(uint32_t coreId);

void
handlePreLaunch();

void
handleDbgBreakInterrupt();

} // namespace debugger
