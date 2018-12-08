#include "log.h"
#include <spdlog/spdlog.h>

Logger gLog;

void
Logger::log(Level lvl, std::string_view msg)
{
   if (mLogger) {
      auto logger = reinterpret_cast<spdlog::logger *>(mLogger.get());
      logger->log(static_cast<spdlog::level::level_enum>(lvl), msg);
   }
}

bool
Logger::should_log(Level level)
{
   if (!mLogger) {
      return false;
   }

   auto logger = reinterpret_cast<spdlog::logger *>(mLogger.get());
   return logger->should_log(static_cast<spdlog::level::level_enum>(level));
}
