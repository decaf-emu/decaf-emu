#pragma once
#include <cstdint>
#include <atomic>
#include <functional>
#include <utility>
#include "state.h"
#include "common/types.h"

struct Tracer;

namespace cpu
{

const uint32_t SRESET_INTERRUPT = 1 << 0;
const uint32_t GENERIC_INTERRUPT = 1 << 1;
const uint32_t ALARM_INTERRUPT = 1 << 2;
const uint32_t DBGBREAK_INTERRUPT = 1 << 3;
const uint32_t GPU_RETIRE_INTERRUPT = 1 << 4;
const uint32_t GPU_FLIP_INTERRUPT = 1 << 5;
const uint32_t FS_DONE_INTERRUPT = 1 << 6;
const uint32_t INTERRUPT_MASK = 0xFFFFFFFF;
const uint32_t NONMASKABLE_INTERRUPTS = SRESET_INTERRUPT;

const uint32_t SYSTEM_BPFLAG = 1 << 0;
const uint32_t USER_BPFLAG = 1 << 1;

enum class jit_mode {
   disabled,
   enabled,
   verify
};

static const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

using EntrypointHandler = void(*)();
using InterruptHandler = void (*)(uint32_t interrupt_flags);
using SegfaultHandler = void(*)(uint32_t address);
using BranchTraceHandler = void(*)(uint32_t target);
using KernelCallFunction = void(*)(Core *state, void *userData);

struct KernelCallEntry
{
   KernelCallFunction func;
   void *user_data;
};

void
initialise();

void
setJitMode(jit_mode mode);

void
setCoreEntrypointHandler(EntrypointHandler handler);

void
setInterruptHandler(InterruptHandler handler);

void
setSegfaultHandler(SegfaultHandler handler);

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

using Tracer = ::Tracer;

Tracer *
allocTracer(size_t size);

void
freeTracer(Tracer *tracer);

void
interrupt(int core_idx,
          uint32_t flags);

bool
clearBreakpoints(uint32_t flags_mask);

bool
addBreakpoint(ppcaddr_t address,
              uint32_t flags);

bool
removeBreakpoint(ppcaddr_t address,
                 uint32_t flags);

uint64_t *
getJitFallbackStats();

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

uint32_t
interruptMask();

uint32_t
setInterruptMask(uint32_t mask);

void
clearInterrupt(uint32_t flags);

void
setNextAlarm(std::chrono::time_point<std::chrono::system_clock> alarm_time);

cpu::Core *
state();

static uint32_t id()
{
   auto core = state();

   if (core) {
      return core->id;
   } else {
      return 0xFF;
   }
}

} // namespace this_core

} // namespace cpu
