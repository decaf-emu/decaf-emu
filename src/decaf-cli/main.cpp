#include <pugixml.hpp>
#include <excmd.h>
#include <iostream>
#include "config.h"
#include "clilog.h"
#include "decaf/decaf.h"

using namespace decaf::input;

#ifdef DECAF_GLFW
int glfwStart();
#endif

#ifdef DECAF_SDL
int sdlStart();
#endif

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
                  allowed<std::string> { {
                     "trace", "debug", "info", "notice", "warning", "error", "critical", "alert", "emerg", "off"
                  } });

   auto sys_options = parser.add_option_group("System Options")
      .add_option("sys-path", description { "Where to locate any external system files." }, value<std::string> {});

   parser.add_command("play")
      .add_option_group(jit_options)
      .add_option_group(log_options)
      .add_option_group(sys_options)
      .add_argument("game directory", value<std::string> {});

   parser.add_command("fuzztest");

   parser.add_command("hwtest")
      .add_option_group(jit_options)
      .add_option_group(log_options);

   return parser;
}

int start(excmd::option_state &options)
{
   // Print version
   if (options.has("version")) {
      // TODO: print git hash
      std::cout << "Decaf Emulator version 0.0.1" << std::endl;
      std::exit(0);
   }

   // Print help
   if (options.has("help")) {
      // TODO: Why do we need to call this a second time? :(
      auto parser = getCommandLineParser();
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
   config::load("config.json");

   // Allow command line options to override config
   if (options.has("jit-debug")) {
      config::jit::debug = true;
   }

   if (options.has("jit")) {
      config::jit::enabled = true;
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
      config::system::system_path = options.get<std::string>("sys-path");
   }

   std::vector<spdlog::sink_ptr> sinks;
   if (config::log::to_stdout) {
      sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
   }
   if (config::log::to_file) {
      sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>("log", "txt", 23, 59, true));
   }

   if (config::log::async) {
      spdlog::set_async_mode(0x1000);
   }

   spdlog::level::level_enum logLevel = spdlog::level::info;
   for (int i = spdlog::level::trace; i <= spdlog::level::off; i++) {
      auto level = static_cast<spdlog::level::level_enum>(i);

      if (spdlog::level::to_str(level) == config::log::level) {
         logLevel = level;
         break;
      }
   }

   decaf::initLogging(sinks, logLevel);
   decaf::setJitMode(config::jit::enabled, config::jit::debug);
   decaf::setSystemPath(config::system::system_path);
   decaf::setGamePath(options.get<std::string>("game directory"));


#ifdef DECAF_GLFW
   if (config::system::platform.compare("glfw") == 0) {
      gLog->info("Using GLFW");
      return glfwStart();
   }
#endif

#ifdef DECAF_SDL
   if (config::system::platform.compare("sdl") == 0) {
      gLog->info("Using SDL");
      return sdlStart();
   }
#endif

   gLog->error("Unsupported platform {}", config::system::platform);
   return -1;
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
   auto parser = getCommandLineParser();
   excmd::option_state options;

   try {
      options = parser.parse(lpCmdLine);
   } catch (excmd::exception ex) {
      std::cout << "Error parsing options: " << ex.what() << std::endl;
      std::exit(-1);
   }

   return start(options);
}
#else
int main(int argc, char **argv) {
   auto parser = getCommandLineParser();
   excmd::option_state options;

   try {
      options = parser.parse(argc, argv);
   } catch (excmd::exception ex) {
      std::cout << "Error parsing options: " << ex.what() << std::endl;
      std::exit(-1);
   }

   return start(options);
}
#endif
