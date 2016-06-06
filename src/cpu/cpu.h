#pragma once
#include <cstdint>
#include <atomic>
#include <functional>
#include <utility>
#include "state.h"
#include "types.h"

struct Tracer;

namespace cpu
{

const uint32_t SRESET_INTERRUPT = 1 << 0;
const uint32_t GENERIC_INTERRUPT = 1 << 1;
const uint32_t ALARM_INTERRUPT = 1 << 2;
const uint32_t DBGBREAK_INTERRUPT = 1 << 3;
const uint32_t GPU_RETIRE_INTERRUPT = 1 << 4;
const uint32_t GPU_FLIP_INTERRUPT = 1 << 5;
const uint32_t INTERRUPT_MASK = 0xFFFFFFFF;
const uint32_t NONMASKABLE_INTERRUPTS = DBGBREAK_INTERRUPT;

const uint32_t SYSTEM_BPFLAG = 1 << 0;
const uint32_t USER_BPFLAG = 1 << 1;

enum class jit_mode {
   enabled,
   disabled,
   debug
};

static const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

typedef void(*entrypoint_handler)();
typedef void(*interrupt_handler)(uint32_t interrupt_flags);
using kernel_call_fn = void(*)(Core *state, void *userData);

struct kernel_call_entry
{
   kernel_call_fn fn;
   void * user_data;
};

void initialise();
void set_jit_mode(jit_mode mode);
void set_core_entrypoint_handler(entrypoint_handler handler);
void set_interrupt_handler(interrupt_handler handler);

uint32_t register_kernel_call(const kernel_call_entry &entry);

void start();
void join();
void halt();

typedef ::Tracer Tracer;
Tracer *alloc_tracer(size_t size);
void free_tracer(Tracer *tracer);

void interrupt(int core_idx, uint32_t flags);

bool clear_breakpoints(uint32_t flags_mask);
bool add_breakpoint(ppcaddr_t address, uint32_t flags);
bool remove_breakpoint(ppcaddr_t address, uint32_t flags);

namespace this_core
{

void set_tracer(Tracer *tracer);

void resume();
void execute_sub();
void wait_for_interrupt();
uint32_t interrupt_mask();
uint32_t set_interrupt_mask(uint32_t mask);
void clear_interrupt(uint32_t flags);

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
