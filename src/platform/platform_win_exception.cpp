#include <vector>
#include "platform.h"
#include "platform_exception.h"
#include "platform_fiber.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>

namespace platform
{

static std::vector<ExceptionHandler>
gExceptionHandlers;

static LONG CALLBACK
exceptionHandler(PEXCEPTION_POINTERS info)
{
   switch (info->ExceptionRecord->ExceptionCode) {
   case STATUS_ACCESS_VIOLATION:
      auto address = info->ExceptionRecord->ExceptionInformation[1];
      auto exception = AccessViolationException { address };

      for (auto &handler : gExceptionHandlers) {
         auto fiber = handler(&exception);

         if (fiber == UnhandledException) {
            // Exception unhandled, try another handler
            continue;
         } else if (fiber == HandledException) {
            // Exception handled, resume execution
            return EXCEPTION_CONTINUE_EXECUTION;
         } else {
            // Exception handled, switch execution to target fiber
            swapToFiber(nullptr, fiber);
         }
      }
      break;
   }

   // Unhandled exception
   return EXCEPTION_CONTINUE_SEARCH;
}

bool
installExceptionHandler(ExceptionHandler handler)
{
   static bool addedHandler = false;

   if (!addedHandler) {
      AddVectoredExceptionHandler(1, exceptionHandler);
      addedHandler = true;
   }

   gExceptionHandlers.push_back(handler);
   return true;
}

} // namespace platform

#endif
