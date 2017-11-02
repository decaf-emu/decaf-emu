#pragma once
#include "mem.h"
#include "state.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <utility>
#include <gsl.h>

struct Tracer;

namespace cpu
{

const uint32_t SRESET_INTERRUPT = 1 << 0;
const uint32_t GENERIC_INTERRUPT = 1 << 1;
const uint32_t ALARM_INTERRUPT = 1 << 2;
const uint32_t DBGBREAK_INTERRUPT = 1 << 3;
const uint32_t GPU_RETIRE_INTERRUPT = 1 << 4;
const uint32_t GPU_FLIP_INTERRUPT = 1 << 5;
const uint32_t IPC_INTERRUPT = 1 << 6;
const uint32_t INTERRUPT_MASK = 0xFFFFFFFF;
const uint32_t NONMASKABLE_INTERRUPTS = SRESET_INTERRUPT;

const uint32_t InvalidCoreId = 0xFF;

enum class jit_mode {
   disabled,
   enabled,
   verify
};

static const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

using EntrypointHandler = std::function<void(Core *core)>;
using InterruptHandler = void (*)(Core *core, uint32_t interrupt_flags);
using SegfaultHandler = void(*)(Core *core, uint32_t address);
using IllInstHandler = void(*)(Core *core);
using BranchTraceHandler = void(*)(Core *core, uint32_t target);
using KernelCallFunction = void(*)(Core *core, void *userData);

struct KernelCallEntry
{
   KernelCallFunction func;
   void *user_data;
};

void
initialise();

void
clearInstructionCache();

void
invalidateInstructionCache(uint32_t address,
                           uint32_t size);

void
addJitReadOnlyRange(ppcaddr_t address, uint32_t size);

void
setCoreEntrypointHandler(EntrypointHandler handler);

void
setInterruptHandler(InterruptHandler handler);

void
setSegfaultHandler(SegfaultHandler handler);

void
setIllInstHandler(IllInstHandler handler);

void
setBranchTraceHandler(BranchTraceHandler handler);

uint32_t
registerKernelCall(const KernelCallEntry &entry);

void
start();

void
join();

void
halt();

std::chrono::steady_clock::time_point
tbToTimePoint(uint64_t ticks);

using Tracer = ::Tracer;

Tracer *
allocTracer(size_t size);

void
freeTracer(Tracer *tracer);

void
interrupt(int core_idx,
          uint32_t flags);

namespace this_core
{

void
setTracer(Tracer *tracer);

void
resume();

void
executeSub();

void
checkInterrupts();

void
waitForInterrupt();

void
waitNextInterrupt(std::chrono::steady_clock::time_point until);

uint32_t
interruptMask();

uint32_t
setInterruptMask(uint32_t mask);

void
clearInterrupt(uint32_t flags);

void
setNextAlarm(std::chrono::steady_clock::time_point alarm_time);

cpu::Core *
state();

static uint32_t id()
{
   auto core = state();

   if (core) {
      return core->id;
   } else {
      return InvalidCoreId;
   }
}

} // namespace this_core

} // namespace cpu
