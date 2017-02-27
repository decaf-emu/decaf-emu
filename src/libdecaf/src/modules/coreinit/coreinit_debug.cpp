#include "coreinit.h"
#include "coreinit_debug.h"
#include "coreinit_enum_string.h"
#include "coreinit_exit.h"
#include "coreinit_sprintf.h"
#include "coreinit_thread.h"
#include "kernel/kernel_loader.h"
#include <common/log.h>

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

int
ENVGetEnvironmentVariable(const char *key,
                          char *buffer,
                          uint32_t length)
{
   if (buffer) {
      *buffer = 0;
   }

   return 0;
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
   auto list = ppctypes::make_va_list(2, 0);
   COSVReport(module, COSReportLevel::Error, fmt, list);
   ppctypes::free_va_list(list);
}

static void
COSWarn(uint32_t module,
        const char *fmt,
        ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(2, 0);
   COSVReport(module, COSReportLevel::Warn, fmt, list);
   ppctypes::free_va_list(list);
}

static void
COSInfo(uint32_t module,
        const char *fmt,
        ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(2, 0);
   COSVReport(module, COSReportLevel::Info, fmt, list);
   ppctypes::free_va_list(list);
}

static void
COSVerbose(uint32_t module,
           const char *fmt,
           ppctypes::VarArgs)
{
   auto list = ppctypes::make_va_list(2, 0);
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
   auto list = ppctypes::make_va_list(3, 0);
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
                uint32_t bufsize)
{
   const char *foundModule = nullptr;
   const char *foundSymbol = nullptr;
   uint32_t foundAddress;

   kernel::loader::lockLoader();
   const auto &modules = kernel::loader::getLoadedModules();

   for (auto &i : modules) {
      auto mod = i.second;
      auto sec = mod->findAddressSection(address);

      if (sec) {
         const char *closestSym = nullptr;
         auto closestAddr = sec->start;
         for (auto &sym : mod->symbols) {
            if (sym.second.address >= closestAddr && sym.second.address <= address) {
               closestSym = sym.first.c_str();
               closestAddr = sym.second.address;
            }
         }
         if (closestSym) {
            foundModule = mod->name.c_str();
            foundSymbol = closestSym;
            foundAddress = closestAddr;
         }
         break;
      }
   }

   kernel::loader::unlockLoader();

   if (foundSymbol) {
      snprintf(buffer, bufsize, "%s|%s", foundModule, foundSymbol);
      return foundAddress;
   } else {
      snprintf(buffer, bufsize, "<unknown>");
      return address;
   }
}

void
Module::registerDebugFunctions()
{
   RegisterKernelFunction(OSIsDebuggerPresent);
   RegisterKernelFunction(OSIsDebuggerInitialized);
   RegisterKernelFunction(ENVGetEnvironmentVariable);
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
