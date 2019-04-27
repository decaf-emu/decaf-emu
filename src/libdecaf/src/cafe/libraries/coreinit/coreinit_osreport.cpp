#include "coreinit.h"
#include "coreinit_cosreport.h"
#include "coreinit_dynload.h"
#include "coreinit_ghs.h"
#include "coreinit_osreport.h"
#include "coreinit_snprintf.h"
#include "coreinit_systeminfo.h"

#include "cafe/cafe_stackobject.h"

#include <common/log.h>
#include <common/strutils.h>

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
   auto msg = fmt::memory_buffer { };
   internal::formatStringV(fmt, vaList, msg);
   free_va_list(vaList);

   internal::OSPanic(file.get(), line,
                     std::string_view { msg.data(), msg.size() });
}

void
OSSendFatalError(virt_ptr<OSFatalError> error,
                 virt_ptr<const char> functionName,
                 uint32_t line)
{
   if (error) {
      if (functionName) {
         string_copy(virt_addrof(error->functionName).get(),
                     error->functionName.size(),
                     functionName.get(),
                     error->functionName.size());
         error->functionName[error->functionName.size() - 1] = char { 0 };
      } else {
         error->functionName[0] = char { 0 };
      }

      error->line = line;
   }

   // TODO: Kernel call 0x6C00 systemFatal
   gLog->error("SystemFatal: messageType:       {}", error->messageType);
   gLog->error("SystemFatal: errorCode:         {}", error->errorCode);
   gLog->error("SystemFatal: internalErrorCode: {}", error->internalErrorCode);
   gLog->error("SystemFatal: processId:         {}", error->processId);
   gLog->error("SystemFatal: functionName:      {}", virt_addrof(error->functionName));
   gLog->error("SystemFatal: line:              {}", error->line);
   ghs_exit(-1);
}

void
OSConsoleWrite(virt_ptr<const char> msg,
               uint32_t size)
{
   gLog->info("[OSConsoleWrite] {}",
              std::string_view { msg.get(), size });
}

namespace internal
{

void
OSPanic(std::string_view file,
        unsigned line,
        std::string_view msg)
{
   auto symbolNameBuffer = StackArray<char, 256> { };
   gLog->error("OSPanic in \"{}\" at line {}: {}.", file, line, msg);

   // Format a guest stack trace
   auto core = cpu::this_core::state();
   auto stackAddress = virt_addr { core->gpr[1] };
   auto stackTraceBuffer = fmt::memory_buffer { };
   fmt::format_to(stackTraceBuffer, "Guest stack trace:\n");

   for (auto i = 0; i < 16; ++i) {
      if (!stackAddress || stackAddress == virt_addr { 0xFFFFFFFF }) {
         break;
      }

      auto backchain = virt_cast<virt_addr *>(stackAddress)[0];
      auto address = virt_cast<virt_addr *>(stackAddress)[1];
      fmt::format_to(stackTraceBuffer, "{}: {}", stackAddress, address);

      auto symbolAddress = OSGetSymbolName(address, symbolNameBuffer, 256);
      if (symbolAddress) {
         fmt::format_to(stackTraceBuffer, " {}+0x{:X}", symbolNameBuffer.get(),
                        static_cast<uint32_t>(address - symbolAddress));
      }

      fmt::format_to(stackTraceBuffer, "\n");
      stackAddress = backchain;
   }

   gLog->error("{}", fmt::to_string(stackTraceBuffer));
   ghs_PPCExit(-1);
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
   RegisterFunctionExport(OSSendFatalError);
   RegisterFunctionExport(OSConsoleWrite);
   RegisterFunctionExportName("__OSConsoleWrite", OSConsoleWrite);
}

} // namespace cafe::coreinit
