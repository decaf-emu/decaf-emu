#pragma once
#include "state.h"

#include <chrono>

namespace cpu
{

const uint32_t SRESET_INTERRUPT = 1 << 0;
const uint32_t GENERIC_INTERRUPT = 1 << 1;
const uint32_t ALARM_INTERRUPT = 1 << 2;
const uint32_t DBGBREAK_INTERRUPT = 1 << 3;
const uint32_t GPU7_INTERRUPT = 1 << 4;
const uint32_t IPC_INTERRUPT = 1 << 5;
const uint32_t INTERRUPT_MASK = 0xFFFFFFFF;
const uint32_t NONMASKABLE_INTERRUPTS = SRESET_INTERRUPT;

const uint32_t InvalidCoreId = 0xFF;

std::chrono::steady_clock::time_point
tbToTimePoint(uint64_t ticks);

void
clearInstructionCache();

void
invalidateInstructionCache(uint32_t address,
                           uint32_t size);

void
addJitReadOnlyRange(uint32_t address,
                    uint32_t size);

void
interrupt(int core_idx,
          uint32_t flags);

namespace this_core
{

void
checkInterrupts();

void
waitForInterrupt();

void
waitNextInterrupt(std::chrono::steady_clock::time_point until = { });

uint32_t
interruptMask();

uint32_t
setInterruptMask(uint32_t mask);

void
clearInterrupt(uint32_t flags);

void
setNextAlarm(std::chrono::steady_clock::time_point alarm_time);

void
resume();

void
executeSub();

cpu::Core*
state();

uint32_t
id();

} // namespace this_core

} // namespace cpu
