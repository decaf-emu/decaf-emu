#include "config.h"
#include "sdl_window.h"

#include <common-sdl/decafsdl_config.h>
#include <common/log.h>
#include <excmd.h>
#include <iostream>
#include <libcpu/cpu.h>
#include <libcpu/mem.h>
#include <libdecaf/decaf.h>
#include <libdecaf/decaf_log.h>
#include <libgpu/gpu_config.h>
#include <spdlog/spdlog.h>

namespace config
{

bool dump_drc_frames = false;
bool dump_tv_frames = false;
std::string dump_frames_dir = "frames";
std::string renderer = "opengl";

} // namespace config


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
   using excmd::make_default_value;

   parser.global_options()
      .add_option("v,version",
                  description { "Show version." })
      .add_option("h,help",
                  description { "Show help." });

   auto replayOptions = parser.add_option_group("Replay Options")
      .add_option("dump-drc-frames",
                  description { "Dump rendered DRC frames to file." })
      .add_option("dump-tv-frames",
                  description { "Dump rendered TV frames to file." })
      .add_option("dump-frames-dir",
                  description { "Folder to place dumped frames in" },
                  make_default_value(config::dump_frames_dir))
      .add_option("renderer",
                  description { "Which graphics renderer to use." },
                  make_default_value(config::renderer));

   parser.add_command("help")
      .add_argument("help-command",
                    optional {},
                    value<std::string> {});

   parser.add_command("replay")
      .add_argument("trace file",
                    value<std::string> {})
      .add_option_group(replayOptions);

   return parser;
}

static int
replay(const std::string &path)
{
   SDLWindow sdl;

   if (!sdl.initCore()) {
      gCliLog->error("Failed to initialise SDL");
      return -1;
   }

   if (config::renderer == "vulkan") {
      if (!sdl.initVulkanGraphics()) {
         gCliLog->error("Failed to initialise Vulkan backend.");
         return -1;
      }
   } else if (config::renderer == "opengl") {
      if (!sdl.initGlGraphics()) {
         gCliLog->error("Failed to initialise OpenGL backend.");
         return -1;
      }
   } else {
      gCliLog->error("Unknown display backend {}", config::renderer);
      return -1;
   }

   return sdl.run(path);
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

   if (options.has("dump-drc-frames")) {
      config::dump_drc_frames = true;
   }

   if (options.has("dump-tv-frames")) {
      config::dump_tv_frames = true;
   }

   if (options.has("dump-frames-dir")) {
      config::dump_frames_dir = options.get<std::string>("dump-frames-dir");
   }

   if (options.has("renderer")) {
      config::renderer = options.get<std::string>("renderer");
   }

   // Always use force_sync for pm4-replay
   config::display::force_sync = true;

   auto traceFile = options.get<std::string>("trace file");

   // Initialise libdecaf logger
   decaf::config::log::to_file = true;
   decaf::config::log::to_stdout = true;
   decaf::config::log::level = "debug";
   decaf::initialiseLogging("pm4-replay.txt");

   auto sinks = gLog->sinks();
   gCliLog = decaf::makeLogger("decaf-pm4-replay");
   gCliLog->set_pattern("[%l] %v");
   gCliLog->info("Trace path {}", traceFile);

   // Let's go boyssssss
   int result = -1;

   // We need to run the replay on a cpu core.
   cpu::initialise();
   cpu::setCoreEntrypointHandler(
      [&](cpu::Core *core) {
         if (core->id == 1) {
            result = replay(traceFile);
         }
      });

   cpu::start();
   cpu::join();

   return result;
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
