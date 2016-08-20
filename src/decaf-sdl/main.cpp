#include "clilog.h"
#include "common/decaf_assert.h"
#include "config.h"
#include "decafsdl.h"
#include "libdecaf/decaf.h"
#include "libdecaf/src/modules/coreinit/coreinit_enum.h"
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
      .add_option("v,version",
                  description { "Show version." })
      .add_option("h,help",
                  description { "Show help." });

   parser.add_command("help")
      .add_argument("help-command",
                    optional {},
                    value<std::string> {});

   auto input_options = parser.add_option_group("input Options")
      .add_option("gamepad-type",
                  description { "Select the input type for the emulated GamePad." },
                  default_value<std::string> { "keyboard" },
                  allowed<std::string> { {
                     "none", "keyboard", "joystick"
                  } });

   auto jit_options = parser.add_option_group("JIT Options")
      .add_option("jit",
                  description { "Enables the JIT engine." })
      .add_option("jit-verify",
                  description { "Verify JIT implementation against interpreter." });

   auto log_options = parser.add_option_group("Log Options")
      .add_option("log-file",
                  description { "Redirect log output to file." })
      .add_option("log-async",
                  description { "Enable asynchronous logging." })
      .add_option("log-no-stdout",
                  description { "Disable logging to stdout." })
      .add_option("log-level",
                  description { "Only display logs with severity equal to or greater than this level." },
                  default_value<std::string> { "trace" },
                  allowed<std::string> { {
                     "trace", "debug", "info", "notice", "warning",
                     "error", "critical", "alert", "emerg", "off"
                  } });

   auto sys_options = parser.add_option_group("System Options")
      .add_option("config",
                  description { "Specify path to configuration file." },
                  value<std::string> {})
      .add_option("display-layout",
                  description { "Set the window display layout." },
                  default_value<std::string> { "split" },
                  allowed<std::string> {
                  {
                     "split", "toggle"
                  } })
      .add_option("display-mode",
                  description{ "Set the window display mode." },
                  default_value<std::string> { "windowed" },
                  allowed<std::string> { {
                     "windowed", "fullscreen"
                  } })
      .add_option("display-stretch",
                  description { "Enable display stretching, aspect ratio will not be maintained." })
      .add_option("region",
                  description { "Set the system region." },
                  default_value<std::string> { "US" },
                  allowed<std::string> { {
                     "EUR", "JAP", "US"
                  } })
      .add_option("sound",
                  description { "Enable sound output." })
      .add_option("sys-path",
                  description { "Where to locate any external system files." },
                  value<std::string> {})
      .add_option("content-path",
                  description { "Sets which path to mount to /vol/content, only set for standalone rpx files." },
                  value<std::string> {})
      .add_option("time-scale",
                  description { "Time scale factor for emulated clock." },
                  default_value<double> { 1.0 });

   parser.add_command("play")
      .add_option_group(input_options)
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
      return path.substr(pos + 1);
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
   std::string configPath, configError;

   if (options.has("config")) {
      configPath = options.get<std::string>("config");
   } else {
      decaf::createConfigDirectory();
      configPath = decaf::makeConfigPath("config.json");
   }

   config::initialize();
   auto configLoaded = config::load(configPath, configError);

   // Allow command line options to override config
   if (options.has("gamepad-type")) {
       auto type = options.get<std::string>("gamepad-type");

       if (type.compare("none") == 0) {
           config::input::vpad0::type = config::input::None;
       } else if (type.compare("keyboard") == 0) {
           config::input::vpad0::type = config::input::Keyboard;
       } else if (type.compare("joystick") == 0) {
           config::input::vpad0::type = config::input::Joystick;
       } else {
           decaf_abort(fmt::format("Invalid input type {}", type));
       }
   }

   if (options.has("jit-verify")) {
      decaf::config::jit::verify = true;
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

   if (options.has("display-mode")) {
       auto mode = options.get<std::string>("display-mode");

       if (mode.compare("windowed") == 0) {
           config::display::mode = config::display::Windowed;
       } else if (mode.compare("fullscreen") == 0) {
          config::display::mode = config::display::Fullscreen;
       } else {
           decaf_abort(fmt::format("Invalid display mode {}", mode));
       }
   }

   if (options.has("display-layout")) {
      auto layout = options.get<std::string>("display-layout");

      if (layout.compare("split") == 0) {
         config::display::layout = config::display::Split;
      } else if (layout.compare("toggle") == 0) {
         config::display::layout = config::display::Toggle;
      } else {
         decaf_abort(fmt::format("Invalid display layout {}", layout));
      }
   }

   if (options.has("display-stretch")) {
       config::display::stretch = true;
   }

   if (options.has("region")) {
      auto region = options.get<std::string>("region");

      if (region.compare("JAP") == 0) {
         decaf::config::system::region = coreinit::SCIRegion::JAP;
      } else if (region.compare("USA") == 0) {
         decaf::config::system::region = coreinit::SCIRegion::USA;
      } else if (region.compare("EUR") == 0) {
         decaf::config::system::region = coreinit::SCIRegion::EUR;
      } else {
         decaf_abort(fmt::format("Invalid region {}", region));
      }
   }

   if (options.has("sys-path")) {
      decaf::config::system::system_path = options.get<std::string>("sys-path");
   }

   if (options.has("content-path")) {
      decaf::config::system::content_path = options.get<std::string>("content-path");
   }

   if (options.has("time-scale")) {
      decaf::config::system::time_scale = options.get<double>("time-scale");
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
   std::vector<spdlog::sink_ptr> cliSinks;
   cliSinks.push_back(spdlog::sinks::stdout_sink_st::instance());
   gCliLog = std::make_shared<spdlog::logger>("decaf-cli", begin(cliSinks), end(cliSinks));
   gCliLog->set_level(logLevel);
   gCliLog->set_pattern("[%l] %v");
   gCliLog->info("Game path {}", gamePath);

   if (configLoaded) {
      gCliLog->info("Loaded config from {}", configPath);
   } else if (configError.empty()) {
      gCliLog->error("Created new config at {}", configPath);
   } else {
      gCliLog->error("Failed to parse config {}:", configPath);
      gCliLog->error("{}", configError);
      gCliLog->error("Try deleting config.json to automatically generate a new one with default settings.");
   }

   DecafSDL sdl;

   if (!sdl.createWindow()) {
      gCliLog->error("Failed to start game");
      return -1;
   }

   if (options.has("sound") && !sdl.initSound()) {
      gCliLog->error("Failed to start game");
      return -1;
   }

   if (!sdl.run(gamePath)) {
      gCliLog->error("Failed to start game");
      return -1;
   }

   return 0;
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
int WINAPI
wWinMain(_In_ HINSTANCE hInstance,
         _In_opt_ HINSTANCE hPrevInstance,
         _In_ LPWSTR lpCmdLine,
         _In_ int nShowCmd)
{
   auto parser = getCommandLineParser();
   excmd::option_state options;

   if (AttachConsole(ATTACH_PARENT_PROCESS)) {
      FILE *dumbFuck;
      freopen_s(&dumbFuck, "CONOUT$", "w", stdout);
      freopen_s(&dumbFuck, "CONOUT$", "w", stderr);
   }

   try {
      options = parser.parse(lpCmdLine);
   } catch (excmd::exception ex) {
      std::cout << "Error parsing options: " << ex.what() << std::endl;
      std::exit(-1);
   }

   return start(parser, options);
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

   return start(parser, options);
}
#endif
