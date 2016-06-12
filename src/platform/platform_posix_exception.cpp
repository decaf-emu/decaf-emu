#include <vector>
#include "platform.h"
#include "platform_exception.h"
#include "platform_fiber.h"
#include "common/log.h"

#ifdef PLATFORM_POSIX
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>

namespace platform
{

static std::vector<ExceptionHandler>
gExceptionHandlers;

static struct sigaction
gOldSegvHandler;

static struct sigaction
gSegvHandler;

static void
segvHandler(int unused_signum, siginfo_t *info, void *context)
{
   auto exception = AccessViolationException { reinterpret_cast<uint64_t>(info->si_addr) };

   for (auto &handler : gExceptionHandlers) {
      auto func = handler(&exception);

      if (func == UnhandledException) {
         // Exception unhandled, try another handler
         continue;
      }

      // Reinstall the handler since SA_RESETHAND will have cleared it.
      sigaction(SIGSEGV, &gSegvHandler, nullptr);

      if (func == HandledException) {
         // Exception handled, resume execution
         return;
      } else {
         // Exception handled, switch execution to target function
         auto ctx = reinterpret_cast<ucontext *>(context);
         ctx->uc_mcontext.gregs[REG_RIP] = reinterpret_cast<uint64_t>(func);
         return;
      }
   }

   sigaction(SIGSEGV, &gOldSegvHandler, nullptr);
}

bool
installExceptionHandler(ExceptionHandler handler)
{
   static bool addedHandler = false;

   if (!addedHandler) {
      gSegvHandler.sa_sigaction = segvHandler;
      sigemptyset(&gSegvHandler.sa_mask);

      // Set SA_RESETHAND so that a SEGV in the handler will terminate the
      // program rather than going into an infinite loop.
      gSegvHandler.sa_flags = SA_SIGINFO | SA_RESETHAND;

      if (sigaction(SIGSEGV, &gSegvHandler, &gOldSegvHandler) != 0) {
         gLog->error("sigaction() failed: {}", strerror(errno));
         return false;
      }

      addedHandler = true;
   }

   gExceptionHandlers.push_back(handler);
   return true;
}

} // namespace platform

#endif
