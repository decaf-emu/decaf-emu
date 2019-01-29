#include "platform.h"

#ifdef PLATFORM_WINDOWS
#include "platform_fiber.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace platform
{

struct Fiber
{
   LPVOID handle = nullptr;
   FiberEntryPoint entry = nullptr;
   void *entryParam = nullptr;
};

Fiber *
getThreadFiber()
{
   auto fiber = new Fiber();
   fiber->handle = ConvertThreadToFiber(NULL);
   return fiber;
}

static void __stdcall
fiberEntryPoint(LPVOID lpFiberParameter)
{
   auto fiber = reinterpret_cast<Fiber *>(lpFiberParameter);
   fiber->entry(fiber->entryParam);
}

Fiber *
createFiber(FiberEntryPoint entry, void *entryParam)
{
   auto fiber = new Fiber();
   fiber->handle = CreateFiber(0, &fiberEntryPoint, fiber);
   fiber->entry = entry;
   fiber->entryParam = entryParam;
   return fiber;
}

void
destroyFiber(Fiber *fiber)
{
   DeleteFiber(fiber->handle);
   delete fiber;
}

void
swapToFiber(Fiber *current, Fiber *target)
{
   SwitchToFiber(target->handle);
}

} // namespace platform

#endif
