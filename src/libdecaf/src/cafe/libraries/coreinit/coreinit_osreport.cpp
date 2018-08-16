#include "coreinit.h"
#include "coreinit_cosreport.h"
#include "coreinit_ghs.h"
#include "coreinit_osreport.h"
#include "coreinit_snprintf.h"
#include "coreinit_systeminfo.h"
#include <common/log.h>

namespace cafe::coreinit
{

void
OSReport(virt_ptr<const char> fmt,
         var_args args)
{
   auto vaList = make_va_list(args);
   COSVReport(COSReportModule::Unknown0, COSReportLevel::Error, fmt, vaList);
   free_va_list(vaList);
}

void
OSReportInfo(virt_ptr<const char> fmt,
             var_args args)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Info) {
      auto vaList = make_va_list(args);
      COSVReport(COSReportModule::Unknown0, COSReportLevel::Info, fmt, vaList);
      free_va_list(vaList);
   }
}

void
OSReportVerbose(virt_ptr<const char> fmt,
                var_args args)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Verbose) {
      auto vaList = make_va_list(args);
      COSVReport(COSReportModule::Unknown0, COSReportLevel::Verbose, fmt, vaList);
      free_va_list(vaList);
   }
}

void
OSReportWarn(virt_ptr<const char> fmt,
             var_args args)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Warn) {
      auto vaList = make_va_list(args);
      COSVReport(COSReportModule::Unknown0, COSReportLevel::Warn, fmt, vaList);
      free_va_list(vaList);
   }
}

void
OSVReport(virt_ptr<const char> fmt,
          virt_ptr<va_list> vaList)
{
   COSVReport(COSReportModule::Unknown0, COSReportLevel::Error, fmt, vaList);
}

void
OSPanic(virt_ptr<const char> file,
        int32_t line,
        virt_ptr<const char> fmt,
        var_args args)
{
   auto vaList = make_va_list(args);
   auto msg = std::string { };
   internal::formatStringV(fmt, vaList, msg);
   free_va_list(vaList);

   internal::OSPanic(file.getRawPointer(), line, msg);
}

void
OSConsoleWrite(virt_ptr<const char> msg,
               uint32_t size)
{
   gLog->info("[OSConsoleWrite] {}",
              std::string_view { msg.getRawPointer(), size });
}

namespace internal
{

void
OSPanic(std::string_view file,
        unsigned line,
        std::string_view msg)
{
   gLog->error("OSPanic in \"{}\" at line {}: {}.", file, line, msg);
   ghs_exit(-1);
}

} // namespace internal

void
Library::registerOsReportSymbols()
{
   RegisterFunctionExport(OSReport);
   RegisterFunctionExport(OSReportWarn);
   RegisterFunctionExport(OSReportInfo);
   RegisterFunctionExport(OSReportVerbose);
   RegisterFunctionExport(OSVReport);
   RegisterFunctionExport(OSPanic);
   RegisterFunctionExport(OSConsoleWrite);
   RegisterFunctionExportName("__OSConsoleWrite", OSConsoleWrite);
}

} // namespace cafe::coreinit
