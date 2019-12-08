#pragma once
#include "mem.h"
#include "state.h"
#include "cpu_control.h"
#include "be2_struct.h"

#include <atomic>
#include <common/platform_stacktrace.h>
#include <cstdint>
#include <functional>
#include <utility>
#include <gsl.h>

struct Tracer;

namespace cpu
{

enum class jit_mode {
   disabled,
   enabled,
   verify
};

static const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

using EntrypointHandler = std::function<void(Core *core)>;
using InterruptHandler = void (*)(Core *core, uint32_t interrupt_flags);
using SegfaultHandler = void(*)(Core *core, uint32_t address, platform::StackTrace *hostStackTrace);
using BranchTraceHandler = void(*)(Core *core, uint32_t target);
using SystemCallHandler = Core * (*)(Core *core, uint32_t id);

void
initialise();

void
setCoreEntrypointHandler(EntrypointHandler handler);

void
setInterruptHandler(InterruptHandler handler);

void
setSegfaultHandler(SegfaultHandler handler);

void
setBranchTraceHandler(BranchTraceHandler handler);

void
setUnknownSystemCallHandler(SystemCallHandler handler);

uint32_t
registerSystemCallHandler(SystemCallHandler handler);

uint32_t
registerIllegalSystemCall();

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

namespace this_core
{

void
setTracer(Tracer *tracer);


} // namespace this_core

} // namespace cpu
