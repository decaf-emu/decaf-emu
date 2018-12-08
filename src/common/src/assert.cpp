#include "decaf_assert.h"
#include "log.h"
#include "platform.h"
#include "platform_stacktrace.h"

#include <iostream>
#include <fmt/format.h>

#ifdef PLATFORM_WINDOWS
#include "platform_winapi_string.h"

#define WIN32_LEAN_AND_MEAN
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

   fmt::memory_buffer out;
   fmt::format_to(out, "Assertion failed:\n");
   fmt::format_to(out, "Expression: {}\n", expression);
   fmt::format_to(out, "File: {}\n", file);
   fmt::format_to(out, "Line: {}\n", line);

   if (!message.empty()) {
      fmt::format_to(out, "Message: {}\n", message);
   }

   if (trace.size()) {
      fmt::format_to(out, "Stacktrace:\n{}\n", trace);
   }

   gLog->critical("{}", out.data());

   std::cerr << out.data() << std::endl;

#ifdef PLATFORM_WINDOWS
   if (IsDebuggerPresent()) {
      OutputDebugStringW(platform::toWinApiString(out.data()).c_str());
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
assertWarnFailed(const char *file,
                 unsigned line,
                 const char *expression,
                 const std::string &message)
{
   gLog->warn("Asserted `{}` ({}) at {}:{}", expression, message, __FILE__, __LINE__);
}

void
hostFaultWithStackTrace(const std::string &fault,
                        platform::StackTrace *stackTrace)
{
   auto trace = platform::formatStackTrace(stackTrace);

   fmt::memory_buffer out;
   fmt::format_to(out, "Encountered host cpu fault:\n");
   fmt::format_to(out, "Fault: {}\n", fault);

   if (trace.size()) {
      fmt::format_to(out, "Stacktrace:\n{}\n", trace);
   }

   gLog->critical("{}", out.data());
   std::cerr << out.data() << std::endl;

#ifdef PLATFORM_WINDOWS
   if (IsDebuggerPresent()) {
      OutputDebugStringW(platform::toWinApiString(out.data()).c_str());
   } else {
      MessageBoxW(NULL,
                  platform::toWinApiString(fault).c_str(),
                  L"Encountered host cpu fault",
                  MB_OK | MB_ICONERROR);
   }
#endif
}
