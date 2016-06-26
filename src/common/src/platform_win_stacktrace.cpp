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

void
printStackTrace(StackTrace *trace)
{
   char printBuf[0x1024];
   // stack allocation neccessary according to MSDN
   SYMBOL_INFO *symbol = new SYMBOL_INFO();
   symbol->MaxNameLen = 255;
   symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

   auto process = GetCurrentProcess();
   SymInitialize(process, NULL, TRUE);

   for (uint16_t i = 0; i < trace->frames; ++i) {
      SymFromAddr(process, (DWORD64)trace->data[i], 0, symbol);
      sprintf_s(printBuf, "%i: %s - 0x%0I64x\n", trace->frames - i - 1, symbol->Name, symbol->Address);
      OutputDebugStringA(printBuf);
   }

   delete symbol;
}

} // namespace platform

#endif
