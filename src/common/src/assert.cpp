#include "decaf_assert.h"
#include "log.h"
#include "platform.h"
#include "platform_stacktrace.h"
#include "platform_winapi_string.h"

#include <iostream>
#include <fmt/format.h>

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#undef NDEBUG
#include <cassert>
#endif

void
assertFailed(const char *file,
             unsigned line,
             const char *expression,
             const std::string &message)
{
   auto stackTrace = platform::captureStackTrace();
   auto trace = platform::formatStackTrace(stackTrace);
   platform::freeStackTrace(stackTrace);

   fmt::MemoryWriter out;
   out << "Assertion failed:\n";
   out << "Expression: " << expression << "\n";
   out << "File: " << file << "\n";
   out << "Line: " << line << "\n";

   if (!message.empty()) {
      out << "Message: " << message << "\n";
   }

   if (trace.size()) {
      out << "Stacktrace:\n" << trace << "\n";
   }

   if (gLog) {
      gLog->critical("{}", out.str());
   }

   std::cerr << out.str() << std::endl;

#ifdef PLATFORM_WINDOWS
   if (IsDebuggerPresent()) {
      OutputDebugStringW(platform::toWinApiString(out.c_str()).c_str());
   } else {
      auto wmsg = platform::toWinApiString(message);
      auto expr = platform::toWinApiString(expression);

      if (!wmsg.empty()) {
         expr += L"\nMessage: ";
         expr += wmsg;
      }

      _wassert(expr.c_str(), platform::toWinApiString(file).c_str(), line);
   }
#endif
}

void
hostFaultWithStackTrace(const std::string &fault,
                        platform::StackTrace *stackTrace)
{
   auto trace = platform::formatStackTrace(stackTrace);

   fmt::MemoryWriter out;
   out << "Encountered host cpu fault:\n";
   out << "Fault: " << fault << "\n";

   if (trace.size()) {
      out << "Stacktrace:\n" << trace << "\n";
   }

   if (gLog) {
      gLog->critical("{}", out.str());
   }

   std::cerr << out.str() << std::endl;

#ifdef PLATFORM_WINDOWS
   if (IsDebuggerPresent()) {
      OutputDebugStringW(platform::toWinApiString(out.c_str()).c_str());
   } else {
      MessageBoxW(NULL,
                  platform::toWinApiString(fault).c_str(),
                  L"Encountered host cpu fault",
                  MB_OK | MB_ICONERROR);
   }
#endif
}
