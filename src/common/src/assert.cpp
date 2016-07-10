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
             const std::string &message)
{
   auto expr = fmt::format("Assertion failed:\n{}:{} {}", file, line, message);

   if (gLog) {
      gLog->critical("{}", expr);
   }

   std::cerr << expr << std::endl;

#ifdef PLATFORM_WINDOWS
   std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
   auto wmsg = converter.from_bytes(message);

   if (IsDebuggerPresent()) {
      auto str = std::wstring { L"Assertion failed:\n" };
      str += wmsg;
      str += '\n';
      OutputDebugStringW(str.c_str());
   } else {
      std::cout << "_wassert" << std::endl;
      auto wfile = converter.from_bytes(file);
      _wassert(wmsg.c_str(), wfile.c_str(), line);
   }
#endif
}
