#include "platform.h"

#ifdef PLATFORM_WINDOWS
#include "platform_exception.h"
#include "platform_fiber.h"

#define WIN32_LEAN_AND_MEAN
#include <vector>
#include <Windows.h>

namespace platform
{

static std::vector<ExceptionHandler>
gExceptionHandlers;

LONG
dispatchException(PEXCEPTION_POINTERS info, Exception *exception)
{
   for (auto &handler : gExceptionHandlers) {
      auto func = handler(exception);

      if (func == UnhandledException) {
         // Exception unhandled, try another handler
         continue;
      } else if (func == HandledException) {
         // Exception handled, resume execution
         return EXCEPTION_CONTINUE_EXECUTION;
      } else {
         // Exception handled, jump to new function
         info->ContextRecord->Rip = reinterpret_cast<DWORD64>(func);
         return EXCEPTION_CONTINUE_EXECUTION;
      }
   }

   return EXCEPTION_CONTINUE_SEARCH;
}

static LONG CALLBACK
exceptionHandler(PEXCEPTION_POINTERS info)
{
   switch (info->ExceptionRecord->ExceptionCode) {
   case STATUS_ACCESS_VIOLATION: {
      auto address = info->ExceptionRecord->ExceptionInformation[1];
      auto exception = AccessViolationException{ address };
      return dispatchException(info, &exception);
   } break;
   case STATUS_ILLEGAL_INSTRUCTION: {
      auto exception = InvalidInstructionException{ };
      return dispatchException(info, &exception);
   } break;
   }

   // Unhandled exception
   return EXCEPTION_CONTINUE_SEARCH;
}

bool
installExceptionHandler(ExceptionHandler handler)
{
   static bool addedHandler = false;

   if (!addedHandler) {
      AddVectoredExceptionHandler(0, exceptionHandler);
      addedHandler = true;
   }

   gExceptionHandlers.push_back(handler);
   return true;
}

} // namespace platform

#endif
