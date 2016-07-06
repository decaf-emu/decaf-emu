#include "platform.h"
#include "platform_stacktrace.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#include <Dbghelp.h>

namespace platform
{

struct StackTrace
{
   void *data[0x100];
   uint16_t frames;
};

struct MySymbol : SYMBOL_INFO
{
   char NameExt[512];
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

#pragma optimize("", off)

void
printStackTrace(StackTrace *trace)
{
   auto process = GetCurrentProcess();

   static bool symInitialise = false;
   if (!symInitialise) {
      SymInitialize(process, NULL, TRUE);
      symInitialise = true;
   }

   // stack allocation neccessary according to MSDN
   auto symbol = new MySymbol();
   symbol->MaxNameLen = 256;
   symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

   char printBuf[0x1024];
   for (uint16_t i = 0; i < trace->frames; ++i) {
      SymFromAddr(process, (DWORD64)trace->data[i], 0, symbol);
      sprintf_s(printBuf, "%i: %s - 0x%0I64x\n", trace->frames - i - 1, (const char*)symbol->Name, symbol->Address);
      OutputDebugStringA(printBuf);
   }

   delete symbol;
}

} // namespace platform

#endif
