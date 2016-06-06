#include "common/log.h"
#include "config.h"

std::shared_ptr<spdlog::logger>
gLog;

namespace logging
{

void
initialise(const std::string &filename)
{
   std::vector<spdlog::sink_ptr> sinks;

   if (config::log::to_stdout) {
      sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
   }

   if (config::log::to_file) {
      sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>(filename, "txt", 23, 59, true));
   }

   if (config::log::async) {
      spdlog::set_async_mode(0x1000);
   }

   gLog = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
   gLog->set_level(spdlog::level::info);

   for (int i = spdlog::level::trace; i <= spdlog::level::off; i++) {
      auto level = static_cast<spdlog::level::level_enum>(i);

      if (spdlog::level::to_str(level) == config::log::level) {
         gLog->set_level(level);
         break;
      }
   }
}

} // namespace logging
