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

struct CoreState;

enum class JitMode {
   Enabled,
   Disabled,
   Debug
};

static const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

void initialise();
void setJitMode(JitMode mode);

typedef void(*interrupt_handler)(CoreState*, ThreadState*);
void set_interrupt_handler(interrupt_handler handler);

void interrupt(CoreState *core);
bool hasInterrupt(CoreState *core);
void clearInterrupt(CoreState *core);

void executeSub(CoreState *core, ThreadState *state);

using KernelCallFn = void(*)(ThreadState *state, void *userData);
using KernelCallEntry = std::pair<KernelCallFn, void*>;

uint32_t
registerKernelCall(const KernelCallEntry &entry);

KernelCallEntry *
getKernelCall(uint32_t id);

} // namespace cpu
