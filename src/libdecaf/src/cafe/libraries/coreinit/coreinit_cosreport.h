#pragma once
#include "coreinit_enum.h"
#include "cafe/cafe_ppc_interface_varargs.h"
#include <cstdarg>
#include <libcpu/be2_struct.h>

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
           const char *fmt,
           std::va_list va);

void
COSError(COSReportModule module,
         const char *fmt,
         ...);

void
COSWarn(COSReportModule module,
        const char *fmt,
        ...);

void
COSInfo(COSReportModule module,
        const char *fmt,
        ...);

void
COSVerbose(COSReportModule module,
           const char *fmt,
           ...);

} // namespace internal

} // namespace cafe::coreinit
