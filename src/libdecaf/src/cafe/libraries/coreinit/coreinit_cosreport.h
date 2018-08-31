#pragma once
#include "coreinit_enum.h"
#include "coreinit_systeminfo.h"

#include "cafe/cafe_ppc_interface_varargs.h"

#include <cstdarg>
#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::coreinit
{

void
COSVReport(COSReportModule module,
           COSReportLevel level,
           virt_ptr<const char> fmt,
           virt_ptr<va_list> vaList);

void
COSError(COSReportModule module,
         virt_ptr<const char> fmt,
         var_args va);

void
COSInfo(COSReportModule module,
        virt_ptr<const char> fmt,
        var_args va);

void
COSVerbose(COSReportModule module,
           virt_ptr<const char> fmt,
           var_args va);

void
COSWarn(COSReportModule module,
        virt_ptr<const char> fmt,
        var_args va);

namespace internal
{

void
COSVReport(COSReportModule module,
           COSReportLevel level,
           const std::string_view &msg);

inline void
COSError(COSReportModule module,
         const std::string_view &msg)
{
   COSVReport(module, COSReportLevel::Error, msg);
}

inline void
COSWarn(COSReportModule module,
        const std::string_view &msg)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Warn) {
      COSVReport(module, COSReportLevel::Warn, msg);
   }
}

inline void
COSInfo(COSReportModule module,
        const std::string_view &msg)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Info) {
      COSVReport(module, COSReportLevel::Info, msg);
   }
}

inline void
COSVerbose(COSReportModule module,
           const std::string_view &msg)
{
   if (OSGetAppFlags().debugLevel() >= OSAppFlagsDebugLevel::Verbose) {
      COSVReport(module, COSReportLevel::Verbose, msg);
   }
}

} // namespace internal

} // namespace cafe::coreinit
