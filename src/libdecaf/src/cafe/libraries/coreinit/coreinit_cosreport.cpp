#include "coreinit.h"
#include "coreinit_cosreport.h"
#include "coreinit_snprintf.h"
#include "coreinit_systeminfo.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/cafe_ppc_interface_varargs.h"

#include <cstdarg>
#include <cstdio>
#include <common/log.h>

namespace cafe::coreinit
{

static void
handleReport(COSReportModule module,
             COSReportLevel level,
             virt_ptr<const char> msg)
{
   // TODO: This actually goes down to a kernel syscall
   switch (level) {
   case COSReportLevel::Error:
      gLog->error("COSVReport {}: {}", module, msg.getRawPointer());
      break;
   case COSReportLevel::Warn:
      gLog->warn("COSVReport {}: {}", module, msg.getRawPointer());
      break;
   case COSReportLevel::Info:
      gLog->info("COSVReport {}: {}", module, msg.getRawPointer());
      break;
   case COSReportLevel::Verbose:
      gLog->debug("COSVReport {}: {}", module, msg.getRawPointer());
      break;
   }
}

void
COSVReport(COSReportModule module,
           COSReportLevel level,
           virt_ptr<const char> fmt,
           virt_ptr<va_list> vaList)
{
   StackArray<char, 128> buffer;
   internal::formatStringV(buffer, buffer.size(), fmt, vaList);
   handleReport(module, level, buffer);
}

void
COSError(COSReportModule module,
         virt_ptr<const char> fmt,
         var_args args)
{
   auto vaList = make_va_list(args);
   COSVReport(module, COSReportLevel::Error, fmt, vaList);
   free_va_list(vaList);
}

void
COSInfo(COSReportModule module,
        virt_ptr<const char> fmt,
        var_args args)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Info) {
      auto vaList = make_va_list(args);
      COSVReport(module, COSReportLevel::Info, fmt, vaList);
      free_va_list(vaList);
   }
}

void
COSVerbose(COSReportModule module,
           virt_ptr<const char> fmt,
           var_args args)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Verbose) {
      auto vaList = make_va_list(args);
      COSVReport(module, COSReportLevel::Verbose, fmt, vaList);
      free_va_list(vaList);
   }
}

void
COSWarn(COSReportModule module,
        virt_ptr<const char> fmt,
        var_args args)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Warn) {
      auto vaList = make_va_list(args);
      COSVReport(module, COSReportLevel::Warn, fmt, vaList);
      free_va_list(vaList);
   }
}

namespace internal
{

void
COSVReport(COSReportModule module,
           COSReportLevel level,
           const char *fmt,
           std::va_list va)
{
   StackArray<char, 128> buffer;
   std::vsnprintf(buffer.getRawPointer(), 128, fmt, va);
   handleReport(module, level, buffer);
}

void
COSError(COSReportModule module,
         const char *fmt,
         ...)
{
   std::va_list va;
   va_start(va, fmt);
   COSVReport(module, COSReportLevel::Error, fmt, va);
}

void
COSWarn(COSReportModule module,
        const char *fmt,
        ...)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Warn) {
      std::va_list va;
      va_start(va, fmt);
      COSVReport(module, COSReportLevel::Warn, fmt, va);
   }
}

void
COSInfo(COSReportModule module,
        const char *fmt,
        ...)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Info) {
      std::va_list va;
      va_start(va, fmt);
      COSVReport(module, COSReportLevel::Info, fmt, va);
   }
}

void
COSVerbose(COSReportModule module,
           const char *fmt,
           ...)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Verbose) {
      std::va_list va;
      va_start(va, fmt);
      COSVReport(module, COSReportLevel::Verbose, fmt, va);
   }
}

} // namespace internal

void
Library::registerCosReportSymbols()
{
   RegisterFunctionExport(COSError);
   RegisterFunctionExport(COSInfo);
   RegisterFunctionExport(COSVerbose);
   RegisterFunctionExport(COSVReport);
   RegisterFunctionExport(COSWarn);
}

} // namespace cafe::coreinit
