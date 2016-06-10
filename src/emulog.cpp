#include "common/log.h"
#include "config.h"
#include "cpu/cpu.h"
#include "modules/coreinit/coreinit_scheduler.h"

std::shared_ptr<spdlog::logger>
gLog;

namespace logging
{

class LogFormatter : public spdlog::formatter
{
public:
   void format(spdlog::details::log_msg& msg) override {

      msg.formatted << '[';

      msg.formatted << spdlog::level::to_str(msg.level);

      msg.formatted << ':';

      auto core = cpu::this_core::state();
      if (core) {
         auto thread = coreinit::internal::getCurrentThread();
         if (thread) {
            msg.formatted.write("w{:01X}{:02X}", core->id, static_cast<uint16_t>(thread->id));
         } else {
            msg.formatted.write("w{:01X}{:02X}", core->id, 0xFF);
         }
      } else {
         msg.formatted << msg.thread_id;
      }

      msg.formatted << "] ";

      msg.formatted << fmt::StringRef(msg.raw.data(), msg.raw.size());
      msg.formatted.write(spdlog::details::os::eol, spdlog::details::os::eol_size);
   }
};

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
   gLog->set_formatter(std::make_shared<LogFormatter>());

   for (int i = spdlog::level::trace; i <= spdlog::level::off; i++) {
      auto level = static_cast<spdlog::level::level_enum>(i);

      if (spdlog::level::to_str(level) == config::log::level) {
         gLog->set_level(level);
         break;
      }
   }
}

} // namespace logging
