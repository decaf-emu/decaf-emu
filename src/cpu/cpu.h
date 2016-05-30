#pragma once
#include <cstdint>
#include <atomic>
#include <functional>
#include <utility>
#include "ppcinvoke.h"
#include "state.h"

struct Tracer;

namespace cpu
{

const uint32_t GENERIC_INTERRUPT = 1 << 0;
const uint32_t ALARM_INTERRUPT = 1 << 1;
const uint32_t DBGBREAK_INTERRUPT = 1 << 2;
const uint32_t GPU_INTERRUPT = 1 << 3;
const uint32_t SRESET_INTERRUPT = 1 << 4;

enum class jit_mode {
   enabled,
   disabled,
   debug
};

static const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

typedef void(*entrypoint_handler)();
typedef void(*interrupt_handler)(uint32_t interrupt_flags);
using kernel_call_fn = void(*)(Core *state, void *userData);
using kernel_call_entry = std::pair<kernel_call_fn, void*>;

void initialise();
void set_jit_mode(jit_mode mode);
void set_core_entrypoint_handler(entrypoint_handler handler);
void set_interrupt_handler(interrupt_handler handler);

uint32_t register_kernel_call(const kernel_call_entry &entry);

void start();
void halt();

typedef ::Tracer Tracer;
Tracer *alloc_tracer(size_t size);
void free_tracer(Tracer *tracer);

void interrupt(int core_idx, uint32_t flags);

namespace this_core
{

void set_tracer(Tracer *tracer);

void resume();
void execute_sub();
void wait_for_interrupt();

void set_next_alarm(std::chrono::time_point<std::chrono::system_clock> alarm_time);

cpu::Core * state();

static uint32_t id() {
   auto core = state();
   if (core) {
      return core->id;
   } else {
      return 0xFF;
   }
}

}

} // namespace cpu
