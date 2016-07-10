#include "decaf_assert.h"
#include "log.h"
#include "platform.h"
#include <iostream>

#ifdef PLATFORM_WINDOWS
#include <cassert>
#include <codecvt>
#include <Windows.h>
#endif

void
assertFailed(const char *file,
             unsigned line,
             const char *expression,
             const std::string &message)
{
   fmt::MemoryWriter out;
   out << "Assertion failed:\n";
   out << "Expression: " << expression << "\n";
   out << "File: " << file << "\n";
   out << "Line: " << line << "\n";

   if (!message.empty()) {
      out << "Message: " << message << "\n";
   }

   if (gLog) {
      gLog->critical("{}", out.str());
   }

   std::cerr << out.str() << std::endl;

#ifdef PLATFORM_WINDOWS
   std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
   auto wmsg = converter.from_bytes(message);

   if (IsDebuggerPresent()) {
      OutputDebugStringW(converter.from_bytes(out.c_str()).c_str());
   } else {
      auto expr = converter.from_bytes(expression);

      if (!wmsg.empty()) {
         expr += L"\nMessage: ";
         expr += wmsg;
      }

      _wassert(expr.c_str(), converter.from_bytes(file).c_str(), line);
   }
#endif
}
