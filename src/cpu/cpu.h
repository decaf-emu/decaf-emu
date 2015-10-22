#pragma once
#include <cstdint>
#include <atomic>
#include <functional>
#include <utility>
#include "state.h"
#include "utils/wfunc_ptr.h"

struct ThreadState;

namespace cpu
{

static const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

enum class JitMode {
   Enabled,
   Disabled,
   Debug
};

struct CoreState;

void setJitMode(JitMode mode);

void initialise();

typedef void(*interrupt_handler)(CoreState*, ThreadState*);
void set_interrupt_handler(interrupt_handler handler);

void interrupt(CoreState *core);
bool hasInterrupt(CoreState *core);
void clearInterrupt(CoreState *core);

void executeSub(CoreState *core, ThreadState *state);

typedef void (*KernelCallFn)(ThreadState *state, void *userData);
typedef std::pair<KernelCallFn, void*> KernelCallEntry;
uint32_t registerKernelCall(KernelCallEntry &entry);
KernelCallEntry * getKernelCall(uint32_t id);

} // namespace cpu

template<typename ReturnType, typename... Args>
ReturnType wfunc_ptr<ReturnType, Args...>::operator()(Args... args)
{
   ThreadState *state = GetCurrentFiberState();

   // Push args
   ppctypes::applyArguments(state, args...);

   // Save state
   auto nia = state->nia;

   // Set state
   state->cia = 0;
   state->nia = address;
   cpu::executeSub(state->core, state);

   // Restore state
   state->nia = nia;

   // Return the result
   return ppctypes::getResult<ReturnType>(state);
}

