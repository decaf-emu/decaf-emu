#include "config.h"
#include "decafcli.h"
#include "libdecaf/decaf.h"
#include <pugixml.hpp>
#include <excmd.h>
#include <iostream>

std::shared_ptr<spdlog::logger>
gCliLog;

using namespace decaf::input;

static excmd::parser
getCommandLineParser()
{
   excmd::parser parser;
   using excmd::description;
   using excmd::optional;
   using excmd::default_value;
   using excmd::allowed;
   using excmd::value;

   parser.global_options()
      .add_option("v,version", description { "Show version." })
      .add_option("h,help", description { "Show help." });

   parser.add_command("help")
      .add_argument("help-command", optional {}, value<std::string> {});

   auto jit_options = parser.add_option_group("JIT Options")
      .add_option("jit", description { "Enables the JIT engine." })
      .add_option("jit-debug", description { "Verify JIT implementation against interpreter." });

   auto log_options = parser.add_option_group("Log Options")
      .add_option("log-file", description { "Redirect log output to file." })
      .add_option("log-async", description { "Enable asynchronous logging." })
      .add_option("log-no-stdout", description { "Disable logging to stdout." })
      .add_option("log-level", description { "Only display logs with severity equal to or greater than this level." },
                  default_value<std::string> { "trace" },
                  allowed<std::string> {
                     {
                        "trace", "debug", "info", "notice", "warning", "error", "critical", "alert", "emerg", "off"
                     } });

   auto sys_options = parser.add_option_group("System Options")
      .add_option("sys-path", description { "Where to locate any external system files." }, value<std::string> {})
      .add_option("timeout_ms", description { "How long to execute the game for before quitting." }, value<uint32_t> {});

   parser.add_command("play")
      .add_option_group(jit_options)
      .add_option_group(log_options)
      .add_option_group(sys_options)
      .add_argument("game directory", value<std::string> {});

   return parser;
}

static std::string
getPathBasename(const std::string &path)
{
   auto pos = path.find_last_of("/\\");

   if (!pos) {
      return path;
   } else {
      return path.substr(pos);
   }
}

int
start(excmd::parser &parser,
      excmd::option_state &options)
{
   // Print version
   if (options.has("version")) {
      // TODO: print git hash
      std::cout << "Decaf Emulator version 0.0.1" << std::endl;
      std::exit(0);
   }

   // Print help
   if (options.empty() || options.has("help")) {
      if (options.has("help-command")) {
         std::cout << parser.format_help("decaf", options.get<std::string>("help-command")) << std::endl;
      } else {
         std::cout << parser.format_help("decaf") << std::endl;
      }

      std::exit(0);
   }

   if (!options.has("play")) {
      return 0;
   }

   // First thing, load the config!
   decaf::createConfigDirectory();
   config::load(decaf::makeConfigPath("cli_config.json"));

   // Allow command line options to override config
   if (options.has("jit-debug")) {
      decaf::config::jit::debug = true;
   }

   if (options.has("jit")) {
      decaf::config::jit::enabled = true;
   }

   if (options.has("log-no-stdout")) {
      config::log::to_stdout = true;
   }

   if (options.has("log-file")) {
      config::log::to_file = true;
   }

   if (options.has("log-async")) {
      config::log::async = true;
   }

   if (options.has("log-level")) {
      config::log::level = options.get<std::string>("log-level");
   }

   if (options.has("sys-path")) {
      decaf::config::system::system_path = options.get<std::string>("sys-path");
   }

   if (options.has("timeout_ms")) {
      config::system::timeout_ms = options.get<uint32_t>("timeout_ms");
   }

   auto gamePath = options.get<std::string>("game directory");
   auto logFile = getPathBasename(gamePath);
   auto logLevel = spdlog::level::info;
   std::vector<spdlog::sink_ptr> sinks;

   if (config::log::to_stdout) {
      sinks.push_back(spdlog::sinks::stdout_sink_st::instance());
   }

   if (config::log::to_file) {
      sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>(logFile, "txt", 23, 59, true));
   }

   if (config::log::async) {
      spdlog::set_async_mode(1024);
   }

   for (int i = spdlog::level::trace; i <= spdlog::level::off; i++) {
      auto level = static_cast<spdlog::level::level_enum>(i);

      if (spdlog::level::to_str(level) == config::log::level) {
         logLevel = level;
         break;
      }
   }

   // Initialise libdecaf logger
   decaf::initialiseLogging(sinks, logLevel);

   // Initialise decaf-cli logger
   gCliLog = std::make_shared<spdlog::logger>("decaf-cli", begin(sinks), end(sinks));
   gCliLog->set_level(logLevel);
   gCliLog->set_pattern("[%l] %v");

   DecafCLI cli;
   return cli.run(gamePath);
}

int main(int argc, char **argv)
{
   auto parser = getCommandLineParser();
   excmd::option_state options;

   try {
      options = parser.parse(argc, argv);
   } catch (excmd::exception ex) {
      std::cout << "Error parsing options: " << ex.what() << std::endl;
      std::exit(-1);
   }

   return start(parser, options);
}
