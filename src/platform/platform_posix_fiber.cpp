#include "platform.h"
#include "platform_fiber.h"
#include "utils/log.h"

#ifdef PLATFORM_POSIX
#include <array>
#include <errno.h>
#include <signal.h>
#include <ucontext.h>

namespace platform
{

static const size_t
DefaultStackSize = 1024 * 1024;

struct Fiber
{
   ucontext_t context;
   FiberEntryPoint entry = nullptr;
   void *entryParam = nullptr;
   std::array<char, DefaultStackSize> stack;
};

static std::function<Fiber *(size_t)> accessViolationHandler;
static struct sigaction oldSegvHandler;
static struct sigaction ourSegvHandler;  // Saved for segvHandler() to use.

static void
segvHandler(int unused_signum, siginfo_t *info, void *unused_context)
{
   Fiber *target = accessViolationHandler(reinterpret_cast<size_t>(info->si_addr));
   if (target) {
      // Reinstall the handler since SA_RESETHAND will have cleared it.
      sigaction(SIGSEGV, &ourSegvHandler, nullptr);

      if (target == resumeCurrentFiber) {
         return;
      } else {
         setcontext(&target->context);
      }
   }

   sigaction(SIGSEGV, &oldSegvHandler, nullptr);
   // On return, the access will be retried, (presumably) causing another
   // SIGSEGV which will this time go to the original handler.
}

bool
installAccessViolationHandler(std::function<Fiber *(size_t)> handler)
{
   accessViolationHandler = handler;

   ourSegvHandler.sa_sigaction = segvHandler;
   sigemptyset(&ourSegvHandler.sa_mask);
   // Set SA_RESETHAND so that a SEGV in the handler will terminate the
   // program rather than going into an infinite loop.
   ourSegvHandler.sa_flags = SA_SIGINFO | SA_RESETHAND;
   if (sigaction(SIGSEGV, &ourSegvHandler, &oldSegvHandler) != 0) {
      gLog->error("sigaction() failed: {}", strerror(errno));
      return false;
   }

   return true;
}

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
