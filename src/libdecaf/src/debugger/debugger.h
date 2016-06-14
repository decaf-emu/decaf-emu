#pragma once
#include <cstdint>
#include "libcpu/state.h"

namespace debugger
{

void initialise();
bool isEnabled();
bool isPaused();

cpu::Core *getPausedCoreState(uint32_t coreId);

void pauseAll();
void resumeAll();
void stepCoreInto(uint32_t coreId);
void stepCoreOver(uint32_t coreId);

void handlePreLaunch();
void handleDbgBreakInterrupt();

} // namespace debugger
