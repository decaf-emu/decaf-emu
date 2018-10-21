#include "decaf_config.h"
#include "decaf_log.h"

#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"
#include "filesystem/filesystem_host_path.h"

#include <common/log.h>
#include <libcpu/cpu.h>
#include <memory>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace decaf
{

// TODO: Move initialiseLogging here and do shit!!!

static std::vector<spdlog::sink_ptr>
sLogSinks;

class GlobalLogFormatter : public spdlog::formatter
{
public:
   virtual void format(const spdlog::details::log_msg &msg, fmt::memory_buffer &dest) override
   {
      auto tm_time = spdlog::details::os::localtime(spdlog::log_clock::to_time_t(msg.time));
      auto duration = msg.time.time_since_epoch();
      auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000000;

      fmt::format_to(dest, "[{:02}:{:02}:{:02}.{:06} {}:",
                     tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, micros,
                     spdlog::level::to_c_str(msg.level));

      auto core = cpu::this_core::state();
      if (core) {
         auto thread = cafe::coreinit::internal::getCurrentThread();
         if (thread) {
            fmt::format_to(dest, "p{:01X} t{:02X}", core->id, thread->id);
         } else {
            fmt::format_to(dest, "p{:01X} t{:02X}", core->id, 0xFF);
         }
      } else if (msg.logger_name) {
         fmt::format_to(dest, "{}", *msg.logger_name);
      } else {
         fmt::format_to(dest, "h{}", msg.thread_id);
      }

      fmt::format_to(dest, "] {}{}",
                     std::string_view { msg.raw.data(), msg.raw.size() },
                     spdlog::details::os::default_eol);
   }

   virtual std::unique_ptr<formatter> clone() const override
   {
      return std::make_unique<GlobalLogFormatter>();
   }
};

static void
initialiseLogSinks(std::string_view filename)
{
   if (decaf::config::log::to_stdout) {
      sLogSinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
   }

   if (decaf::config::log::to_file) {
      auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      auto time = std::localtime(&now);

      auto logFilename =
         fmt::format("{}_{}-{:02}-{:02}_{:02}-{:02}-{:02}.txt",
                     filename,
                     time->tm_year + 1900, time->tm_mon, time->tm_mday,
                     time->tm_hour, time->tm_min, time->tm_sec);

      auto path = fs::HostPath { decaf::config::log::directory }.join(logFilename);
      sLogSinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.path()));
   }

   if (decaf::config::log::async) {
      spdlog::init_thread_pool(1024, 1);
   }
}

static void
initialiseGlobalLogger()
{
   auto logLevel = spdlog::level::from_str(decaf::config::log::level);

   if (decaf::config::log::async) {
      gLog = std::make_shared<spdlog::async_logger>("decaf",
                                                    std::begin(sLogSinks),
                                                    std::end(sLogSinks),
                                                    spdlog::thread_pool());
   } else {
      gLog = std::make_shared<spdlog::logger>("decaf",
                                              std::begin(sLogSinks),
                                              std::end(sLogSinks));
      gLog->flush_on(spdlog::level::trace);
   }

   gLog->set_level(logLevel);
   gLog->set_formatter(std::make_unique<GlobalLogFormatter>());
   spdlog::register_logger(gLog);
}

void
initialiseLogging(std::string_view filename)
{
   initialiseLogSinks(filename);
   initialiseGlobalLogger();
}

std::shared_ptr<spdlog::logger>
makeLogger(std::string name,
           std::vector<spdlog::sink_ptr> sinks)
{
   auto logger = std::shared_ptr<spdlog::logger> { };
   sinks.insert(sinks.end(), sLogSinks.begin(), sLogSinks.end());

   if (decaf::config::log::async) {
      logger = std::make_shared<spdlog::async_logger>(name,
                                                      std::begin(sinks),
                                                      std::end(sinks),
                                                      spdlog::thread_pool());
   } else {
      logger = std::make_shared<spdlog::logger>(name,
                                                std::begin(sinks),
                                                std::end(sinks));
      logger->flush_on(spdlog::level::trace);
   }

   auto logLevel = spdlog::level::from_str(decaf::config::log::level);
   logger->set_level(logLevel);
   logger->set_formatter(std::make_unique<GlobalLogFormatter>());
   spdlog::register_logger(logger);
   return logger;
}

void
setLogLevel(spdlog::level::level_enum level)
{
   spdlog::apply_all(
      [=](std::shared_ptr<spdlog::logger> logger) {
         logger->set_level(level);
      });

   decaf::config::log::level = spdlog::level::to_c_str(level);
}

} // namespace decaf
