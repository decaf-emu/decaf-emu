#include <vector>
#include "platform.h"
#include "platform_exception.h"
#include "platform_fiber.h"
#include "log.h"

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
sExceptionHandlers;

static struct sigaction
sSegvHandler;

static struct sigaction
sSystemSegvHandler;

static struct sigaction
sIllHandler;

static struct sigaction
sSystemIllHandler;

static void
dispatchException(Exception *exception,
                  void *context,
                  int signum,
                  const struct sigaction *ourHandler,
                  const struct sigaction *sysHandler)
{
   // Reset to the original signal handler in case an exception handler
   //  generates a signal of its own
   sigaction(signum, sysHandler, nullptr);

   static bool sInSignal = false;

   // Avoid recursive signal handling (in case an exception handler looking
   //  at a SIGILL causes a SIGSEGV, for example)
   if (sInSignal) {
      return;
   }

   sInSignal = true;

   for (auto &handler : sExceptionHandlers) {
      auto func = handler(exception);

      if (func == UnhandledException) {
         // Exception unhandled, try another handler
         continue;
      }

      sInSignal = false;

      // Reinstall our signal handler
      sigaction(signum, ourHandler, nullptr);

      if (func == HandledException) {
         // Exception handled, resume execution
         return;
      } else {
         // Exception handled, switch execution to target function
         auto ctx = reinterpret_cast<ucontext_t *>(context);
#ifdef PLATFORM_APPLE
         ctx->uc_mcontext->__ss.__rip = reinterpret_cast<uint64_t>(func);
#else
         ctx->uc_mcontext.gregs[REG_RIP] = reinterpret_cast<uint64_t>(func);
#endif
         return;
      }
   }

   // No exception handlers, found, so re-run the failing instruction to
   //  call the original signal handler
   return;
}

static void
segvHandler(int signum, siginfo_t *info, void *context)
{
   auto exception = AccessViolationException { reinterpret_cast<uint64_t>(info->si_addr) };
   dispatchException(&exception, context, signum, &sSegvHandler, &sSystemSegvHandler);
}

static void
illHandler(int signum, siginfo_t *info, void *context)
{
   auto exception = InvalidInstructionException { };
   dispatchException(&exception, context, signum, &sIllHandler, &sSystemIllHandler);
}

bool
installExceptionHandler(ExceptionHandler handler)
{
   static bool addedHandlers = false;

   if (!addedHandlers) {
      sigemptyset(&sSegvHandler.sa_mask);

      // Set SA_RESETHAND so that a SEGV in the handler will terminate the
      // program rather than going into an infinite loop.
      sSegvHandler.sa_flags = SA_SIGINFO | SA_RESETHAND;

      sSegvHandler.sa_sigaction = segvHandler;
      if (sigaction(SIGSEGV, &sSegvHandler, &sSystemSegvHandler) != 0) {
         gLog->error("sigaction(SIGSEGV) failed: {}", strerror(errno));
         return false;
      }

      sIllHandler = sSegvHandler;
      sIllHandler.sa_sigaction = illHandler;
      if (sigaction(SIGILL, &sIllHandler, &sSystemIllHandler) != 0) {
         gLog->error("sigaction(SIGILL) failed: {}", strerror(errno));
         return false;
      }

      addedHandlers = true;
   }

   sExceptionHandlers.push_back(handler);
   return true;
}

} // namespace platform

#endif
