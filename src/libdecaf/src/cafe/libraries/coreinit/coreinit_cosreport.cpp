#include "coreinit.h"
#include "coreinit_cosreport.h"
#include "coreinit_snprintf.h"
#include "coreinit_systeminfo.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/cafe_ppc_interface_varargs.h"

#include <common/strutils.h>
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
      gLog->error("COSVReport {}: {}", module, msg.get());
      break;
   case COSReportLevel::Warn:
      gLog->warn("COSVReport {}: {}", module, msg.get());
      break;
   case COSReportLevel::Info:
      gLog->info("COSVReport {}: {}", module, msg.get());
      break;
   case COSReportLevel::Verbose:
      gLog->debug("COSVReport {}: {}", module, msg.get());
      break;
   }
}

void
COSVReport(COSReportModule module,
           COSReportLevel level,
           virt_ptr<const char> fmt,
           virt_ptr<va_list> vaList)
{
   auto buffer = StackArray<char, 128> { };
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
           const std::string_view &msg)
{
   auto buffer = StackArray<char, 128> { };
   string_copy(buffer.get(), msg.data(), buffer.size());
   buffer[127] = char { 0 };
   handleReport(module, level, buffer);
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
