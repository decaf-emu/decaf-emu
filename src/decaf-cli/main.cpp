#include "config.h"
#include "decafcli.h"

#include <common/decaf_assert.h>
#include <common/log.h>
#include <common/platform_dir.h>
#include <excmd.h>
#include <iostream>
#include <libconfig/config_excmd.h>
#include <libconfig/config_toml.h>
#include <libdecaf/decaf.h>
#include <libdecaf/decaf_log.h>
#include <spdlog/sinks/stdout_sinks.h>

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

   auto cli_options = parser.add_option_group("Cli Options")
      .add_option("config",
                  description { "Specify path to configuration file." },
                  value<std::string> {})
      .add_option("timeout_ms",
                  description { "How long to execute the game for before quitting." },
                  value<uint32_t> {});

   auto config_options = config::getExcmdGroups(parser);

   auto cmdPlay = parser.add_command("play")
      .add_option_group(cli_options)
      .add_argument("game directory", value<std::string> {});

   for (auto group : config_options) {
      cmdPlay.add_option_group(group);
   }

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

   auto gamePath = options.get<std::string>("game directory");

   // Load config file
   std::string configPath, configError;

   if (options.has("config")) {
      configPath = options.get<std::string>("config");
   } else {
      decaf::createConfigDirectory();
      configPath = decaf::makeConfigPath("cli-config.toml");
   }

   // If config file does not exist, create a default one.
   if (!platform::fileExists(configPath)) {
      auto toml = cpptoml::make_table();
      config::saveToTOML(toml);
      config::saveFrontendToml(toml);
      std::ofstream out { configPath };
      out << (*toml);
   }

   try {
      auto toml = cpptoml::parse_file(configPath);
      config::loadFromTOML(toml);
      config::loadFrontendToml(toml);
   } catch (cpptoml::parse_exception ex) {
      configError = ex.what();
   }

   config::loadFromExcmd(options);

   // Allow command line options to override config
   if (options.has("timeout_ms")) {
      config::system::timeout_ms = options.get<uint32_t>("timeout_ms");
   }

   // Initialise libdecaf logger
   auto logFile = getPathBasename(gamePath);
   decaf::initialiseLogging(logFile);

   // Initialise frontend logger
   if (!decaf::config::log::to_stdout) {
      // Always do cli log to stdout
      gCliLog = decaf::makeLogger("decaf-cli",
                                  { std::make_shared<spdlog::sinks::stdout_sink_mt>() });
   } else {
      gCliLog = decaf::makeLogger("decaf-cli");
   }

   gCliLog->set_pattern("[%l] %v");
   gCliLog->info("Game path {}", gamePath);

   if (configError.empty()) {
      gCliLog->info("Loaded config from {}", configPath);
   } else {
      gCliLog->error("Failed to parse config {}: {}", configPath, configError);
   }

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
