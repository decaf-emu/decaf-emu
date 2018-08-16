#pragma once
#include "cafe_loader_entry.h"
#include <common/log.h>

namespace cafe::loader::internal
{

template<typename... Args>
void
Loader_ReportError(const char *fmt, Args... args)
{
   gLog->error(fmt, args...);
}

template<typename... Args>
void
Loader_ReportWarn(const char *fmt, Args... args)
{
   if (getProcFlags().debugLevel() >= kernel::DebugLevel::Warn) {
      gLog->warn(fmt, args...);
   }
}

template<typename... Args>
void
Loader_ReportInfo(const char *fmt, Args... args)
{
   if (getProcFlags().debugLevel() >= kernel::DebugLevel::Info) {
      gLog->info(fmt, args...);
   }
}

template<typename... Args>
void
Loader_ReportNotice(const char *fmt, Args... args)
{
   if (getProcFlags().debugLevel() >= kernel::DebugLevel::Notice) {
      gLog->debug(fmt, args...);
   }
}

template<typename... Args>
void
Loader_ReportVerbose(const char *fmt, Args... args)
{
   if (getProcFlags().debugLevel() >= kernel::DebugLevel::Verbose) {
      gLog->trace(fmt, args...);
   }
}

template<typename... Args>
void
Loader_LogEntry(uint32_t unk1, uint32_t unk2, uint32_t unk3, const char *fmt, Args... args)
{
   gLog->debug(fmt, args...);
}

template<typename... Args>
void
Loader_Panic(uint32_t unk1, const char *fmt, Args... args)
{
   gLog->error(fmt, args...);
   decaf_abort("Loader_Panic");
}

template<typename... Args>
void
LiPanic(const char *file, int line, const char *fmt, Args... args)
{
   gLog->error(fmt, args...);
   gLog->error("Loader_Panic: {}, line {}", file, line);
   Loader_Panic(0x130016, "in \"{}\" on line {}.", file, line);
}

} // namespace cafe::loader::internal
