#include "coreinit.h"
#include "coreinit_debug.h"
#include "coreinit_enum_string.h"
#include "coreinit_exit.h"
#include "coreinit_sprintf.h"
#include "coreinit_thread.h"
#include "kernel/kernel_loader.h"
#include "common/log.h"

namespace coreinit
{

BOOL
OSIsDebuggerPresent()
{
   return FALSE;
}

BOOL
OSIsDebuggerInitialized()
{
   return FALSE;
}

static void
COSVReport(uint32_t module,
           COSReportLevel level,
           const char *fmt,
           ppctypes::va_list *list)
{
   auto str = std::string { };
   internal::formatStringV(fmt, list, str);
   gLog->info("[COSVReport {} {}] {}", module, enumAsString(level), str);
}

static void
COSError(uint32_t module,
         const char *fmt,
         ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(1, 0);
   COSVReport(module, COSReportLevel::Error, fmt, list);
   ppctypes::free_va_list(list);
}

static void
COSWarn(uint32_t module,
        const char *fmt,
        ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(1, 0);
   COSVReport(module, COSReportLevel::Warn, fmt, list);
   ppctypes::free_va_list(list);
}

static void
COSInfo(uint32_t module,
        const char *fmt,
        ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(1, 0);
   COSVReport(module, COSReportLevel::Info, fmt, list);
   ppctypes::free_va_list(list);
}

static void
COSVerbose(uint32_t module,
           const char *fmt,
           ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(1, 0);
   COSVReport(module, COSReportLevel::Verbose, fmt, list);
   ppctypes::free_va_list(list);
}

static void
OSVReport(const char *fmt,
          ppctypes::va_list *list)
{
   COSVReport(0, COSReportLevel::Error, fmt, list);
}

static void
OSReport(const char *fmt,
         ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(1, 0);
   COSVReport(0, COSReportLevel::Error, fmt, list);
   ppctypes::free_va_list(list);
}

static void
OSReportWarn(const char *fmt,
             ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(1, 0);
   COSVReport(0, COSReportLevel::Warn, fmt, list);
   ppctypes::free_va_list(list);
}

static void
OSReportInfo(const char *fmt,
             ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(1, 0);
   COSVReport(0, COSReportLevel::Info, fmt, list);
   ppctypes::free_va_list(list);
}

static void
OSReportVerbose(const char *fmt,
                ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(1, 0);
   COSVReport(0, COSReportLevel::Verbose, fmt, list);
   ppctypes::free_va_list(list);
}

static void
OSPanic(const char *file,
        int line,
        const char *fmt,
        ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(1, 0);
   auto str = std::string {};
   internal::formatStringV(fmt, list, str);
   ppctypes::free_va_list(list);

   gLog->error("[OSPanic] {}:{} {}", file, line, str);
   ghs_exit(-1);
}

static void
OSConsoleWrite(const char *msg,
               uint32_t size)
{
   auto str = std::string { msg, size };
   gLog->info("[OSConsoleWrite] {}", str);
}

static uint32_t
OSGetSymbolName(uint32_t address,
                char *buffer,
                int bufsize)
{
   auto retval = 0u;
   auto found = false;

   kernel::loader::lockLoader();
   const auto &modules = kernel::loader::getLoadedModules();

   for (auto &mod : modules) {
      auto codeBase = 0u;

      for (auto &sec : mod.second->sections) {
         if (sec.name.compare(".text") == 0) {
            codeBase = sec.start;
            break;
         }
      }

      for (auto &sym : mod.second->symbols) {
         if (sym.second.address == address) {
            strncpy(buffer, sym.first.c_str(), bufsize);
            retval = codeBase;
            found = true;
            break;
         }
      }

      if (found) {
         break;
      }
   }

   kernel::loader::unlockLoader();
   return retval;
}

void
Module::registerDebugFunctions()
{
   RegisterKernelFunction(OSIsDebuggerPresent);
   RegisterKernelFunction(OSIsDebuggerInitialized);
   RegisterKernelFunction(COSVReport);
   RegisterKernelFunction(COSError);
   RegisterKernelFunction(COSWarn);
   RegisterKernelFunction(COSInfo);
   RegisterKernelFunction(COSVerbose);
   RegisterKernelFunction(OSVReport);
   RegisterKernelFunction(OSReport);
   RegisterKernelFunction(OSReportWarn);
   RegisterKernelFunction(OSReportInfo);
   RegisterKernelFunction(OSReportVerbose);
   RegisterKernelFunction(OSPanic);
   RegisterKernelFunction(OSConsoleWrite);
   RegisterKernelFunction(OSGetSymbolName);
}

} // namespace coreinit
