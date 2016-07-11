#include "platform.h"
#include "platform_fiber.h"
#include "log.h"

#ifdef PLATFORM_POSIX
#include <array>
#include <errno.h>
#include <signal.h>
#include <ucontext.h>

#ifdef DECAF_VALGRIND
   #include <valgrind/valgrind.h>
#endif

namespace platform
{

static const size_t
DefaultStackSize = 1024 * 1024;

struct Fiber
{
   ucontext_t context;
   FiberEntryPoint entry = nullptr;
   void *entryParam = nullptr;
#ifdef DECAF_VALGRIND
   unsigned int valgrindStackId;
#endif
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

#ifdef DECAF_VALGRIND
   fiber->valgrindStackId = VALGRIND_STACK_REGISTER(&fiber->stack[0], &fiber->stack[fiber->stack.size() - 1]);
#endif

   getcontext(&fiber->context);
   fiber->context.uc_stack.ss_sp = &fiber->stack[0];
   fiber->context.uc_stack.ss_size = fiber->stack.size();
   fiber->context.uc_link = nullptr;

   makecontext(&fiber->context, reinterpret_cast<void(*)()>(&fiberEntryPoint), 1, fiber);
   return fiber;
}

void
destroyFiber(Fiber *fiber)
{
#ifdef DECAF_VALGRIND
   VALGRIND_STACK_DEREGISTER(fiber->valgrindStackId);
#endif

   delete fiber;
}

void
swapToFiber(Fiber *current, Fiber *target)
{
   if (!current) {
      setcontext(&target->context);
   } else {
      swapcontext(&current->context, &target->context);
   }
}

} // namespace platform

#endif
