#pragma once
#include <functional>

namespace platform
{

struct Fiber;
using FiberEntryPoint = std::function<void(void *)>;

// Magic value used in the access violation handler.
static Fiber * const resumeCurrentFiber = reinterpret_cast<Fiber *>((uintptr_t)-1);

Fiber *
getThreadFiber();

Fiber *
createFiber(FiberEntryPoint entry, void *entryParam);

void
destroyFiber(Fiber *fiber);

void
swapToFiber(Fiber *current, Fiber *target);

// The handler should return a fiber to which to switch, the special value
// resumeCurrentFiber to resume the current fiber, or null for the system
// default behavior (program termination).
bool
installAccessViolationHandler(std::function<Fiber *(size_t)> handler);

} // namespace platform
