#pragma once
#include <cstdint>

namespace debugger
{

void initialise();
bool isEnabled();
bool isPaused();

void pauseAll();
void resumeAll();
void stepCoreInto(uint32_t coreId);
void stepCoreOver(uint32_t coreId);

void handlePreLaunch();
void handleDbgBreakInterrupt();

} // namespace debugger
