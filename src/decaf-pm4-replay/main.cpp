#include "sdl_window.h"
#include <excmd.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <libdecaf/decaf.h>

std::shared_ptr<spdlog::logger>
gCliLog;

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

   parser.add_command("replay")
      .add_argument("trace file", value<std::string> {});

   return parser;
}

int
start(excmd::parser &parser,
      excmd::option_state &options)
{
   // Print version
   if (options.has("version")) {
      // TODO: print git hash
      std::cout << "Decaf PM4 Replay tool version 0.0.1" << std::endl;
      std::exit(0);
   }

   // Print help
   if (options.empty() || options.has("help")) {
      if (options.has("help-command")) {
         std::cout << parser.format_help("decaf-pm4-replay", options.get<std::string>("help-command")) << std::endl;
      } else {
         std::cout << parser.format_help("decaf-pm4-replay") << std::endl;
      }

      std::exit(0);
   }

   if (!options.has("replay")) {
      return 0;
   }

   auto traceFile = options.get<std::string>("trace file");

   std::vector<spdlog::sink_ptr> sinks;
   sinks.push_back(spdlog::sinks::stdout_sink_st::instance());
   sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>("pm4-replay", "txt", 23, 59, true));

   gCliLog = std::make_shared<spdlog::logger>("decaf-pm4-replay", begin(sinks), end(sinks));
   gCliLog->set_level(spdlog::level::debug);
   gCliLog->set_pattern("[%l] %v");
   gCliLog->info("Trace path {}", traceFile);

   // Initialise libdecaf logger
   decaf::initialiseLogging(sinks, spdlog::level::debug);

   // Let's go boyssssss
   SDLWindow window;

   if (!window.createWindow()) {
      return -1;
   }

   return window.run(traceFile) ? 0 : -1;
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
