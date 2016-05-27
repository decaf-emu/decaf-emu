#pragma once
#include <cstdint>
#include <atomic>
#include <functional>
#include <utility>
#include "ppcinvoke.h"
#include "state.h"

struct ThreadState;

namespace cpu
{

const uint32_t GENERIC_INTERRUPT = 1 << 0;
const uint32_t ALARM_INTERRUPT = 1 << 1;
const uint32_t DBGBREAK_INTERRUPT = 1 << 2;
const uint32_t GPU_INTERRUPT = 1 << 3;
const uint32_t SRESET_INTERRUPT = 1 << 4;

enum class JitMode {
   Enabled,
   Disabled,
   Debug
};

static const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

void initialise();
void setJitMode(JitMode mode);

void start();
void halt();

void core_resume();
void core_execute_sub();
void core_wait_for_interrupt();

void core_set_next_alarm(std::chrono::time_point<std::chrono::system_clock> alarm_time);

Core * get_current_core();

static uint32_t get_current_core_id() {
   auto core = get_current_core();
   if (core) {
      return core->id;
   } else {
      return 0xFF;
   }
}

void interrupt(int coreIdx, uint32_t flags);

typedef void(*interrupt_handler)(uint32_t interrupt_flags);
void set_interrupt_handler(interrupt_handler handler);

typedef void(*entrypoint_handler)();
void set_core_entrypoint_handler(entrypoint_handler handler);

void setRoundingMode(ThreadState *state);

using KernelCallFn = void(*)(ThreadState *state, void *userData);
using KernelCallEntry = std::pair<KernelCallFn, void*>;

uint32_t
registerKernelCall(const KernelCallEntry &entry);

KernelCallEntry *
getKernelCall(uint32_t id);

} // namespace cpu
