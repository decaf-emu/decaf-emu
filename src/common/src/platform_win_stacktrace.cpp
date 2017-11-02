#include "platform.h"
#include "platform_stacktrace.h"
#include <fmt/format.h>

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#include <Dbghelp.h>
#include <memory>

namespace platform
{

struct StackTrace
{
   void *data[0x100];
   uint16_t frames;
};

struct MySymbol : SYMBOL_INFO
{
   CHAR NameExt[MAX_SYM_NAME];
};

StackTrace *
captureStackTrace()
{
   auto trace = new StackTrace();
   trace->frames = CaptureStackBackTrace(0, 0x100, trace->data, NULL);
   return trace;
}

void
freeStackTrace(StackTrace *trace)
{
   delete trace;
}

std::string
formatStackTrace(StackTrace *trace)
{
   auto process = GetCurrentProcess();
   static bool symInitialise = false;

   if (!symInitialise) {
      SymInitialize(process, NULL, TRUE);
      symInitialise = true;
   }

   auto symbol = std::make_unique<MySymbol>();
   symbol->MaxNameLen = MAX_SYM_NAME;
   symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

   fmt::MemoryWriter out;

   for (auto i = 0u; i < trace->frames; ++i) {
      SymFromAddr(process, (DWORD64)trace->data[i], 0, symbol.get());
      out.write("{}: {} - 0x{:X}x\n", trace->frames - i - 1, (const char*)symbol->Name, symbol->Address);
   }

   return out.str();
}

void
printStackTrace(StackTrace *trace)
{
   auto str = formatStackTrace(trace);
   OutputDebugStringA(str.c_str());
}

} // namespace platform

#endif
