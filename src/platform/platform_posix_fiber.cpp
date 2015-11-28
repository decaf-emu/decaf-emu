#include "platform.h"
#include "platform_fiber.h"

#ifdef PLATFORM_POSIX
#include <array>
#include <ucontext.h>

namespace platform
{

static size_t
DefaultStackSize = 1024 * 1024;

struct Fiber
{
   ucontext_t context;
   FiberEntryPoint entry = nullptr;
   void *entryParam = nullptr;
   std::array<char, DefaultStackSize> stack;
};

Fiber *
getThreadFiber()
{
   auto fiber = new Fiber();
   return fiber;
}

static void
fiberEntryPoint(Fiber *fiber)
{
   fiber->entry(fiber->entryParam);
}

Fiber *
createFiber(FiberEntryPoint entry, void *entryParam)
{
   auto fiber = new Fiber();
   fiber->entry = entry;
   fiber->entryParam = entryParam;

   getcontext(&fiber->context);
   fiber->context.uc_stack.ss_sp = &fiber->stack[0];
   fiber->context.uc_stack.ss_size = fiber->stack.size();
   fiber->context.uc_link = NULL;

   makecontext(&fiber->context, reinterpret_cast<void(*)()>(&fiberEntryPoint), 1, fiber);
   return fiber;
}

void
destroyFiber(Fiber *fiber)
{
   delete fiber;
}

void
swapToFiber(Fiber *current, Fiber *target)
{
   swapcontext(&current->context, &target->context);
}

} // namespace platform

#endif
