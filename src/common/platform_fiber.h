#pragma once
#include <functional>

namespace platform
{

struct Fiber;

using FiberEntryPoint = std::function<void(void *)>;

Fiber *
getThreadFiber();

Fiber *
createFiber(FiberEntryPoint entry,
            void *entryParam);

void
destroyFiber(Fiber *fiber);

void
swapToFiber(Fiber *current,
            Fiber *target);

} // namespace platform
