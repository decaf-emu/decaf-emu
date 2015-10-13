#include <pugixml.hpp>
#include <docopt.h>
#include "bitutils.h"
#include "codetests.h"
#include "filesystem/filesystem.h"
#include "instructiondata.h"
#include "interpreter.h"
#include "processor.h"
#include "jit.h"
#include "loader.h"
#include "log.h"
#include "memory.h"
#include "modules/gameloader/gameloader.h"
#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/erreula/erreula.h"
#include "modules/gx2/gx2.h"
#include "modules/nn_ac/nn_ac.h"
#include "modules/nn_act/nn_act.h"
#include "modules/nn_fp/nn_fp.h"
#include "modules/nn_save/nn_save.h"
#include "modules/nn_temp/nn_temp.h"
#include "modules/zlib125/zlib125.h"
#include "modules/proc_ui/proc_ui.h"
#include "modules/padscore/padscore.h"
#include "modules/sysapp/sysapp.h"
#include "system.h"
#include "usermodule.h"
#include "platform.h"
#include "trace.h"
#include "teenyheap.h"
#include "debugger.h"

std::shared_ptr<spdlog::logger>
gLog;

void initialiseEmulator();
bool test(const std::string &as, const std::string &path);
bool play(const fs::HostPath &path);

static const char USAGE[] =
R"(WiiU Emulator

Usage:
   wiiu play [--jit | --jitdebug] [--logfile] [--log-async] [--log-level=<log-level>] <game directory>
   wiiu test [--jit | --jitdebug] [--logfile] [--log-async] [--log-level=<log-level>] [--as=<ppcas>] <test directory>
   wiiu (-h | --help)
   wiiu --version

Options:
   -h --help     Show this screen.
   --version     Show version.
   --jit         Enables the JIT engine.
   --logfile     Redirect log output to file.
   --log-async   Enable asynchronous logging.
   --log-level=<log-level> [default: trace]
                  Only display logs with severity equal to or greater than this level.
                  Available levels: trace, debug, info, notice, warning, error, critical, alert, emerg, off
   --as=<ppcas>  Path to PowerPC assembler [default: powerpc-eabi-as.exe].
)";

static const std::string&
getGameName(const fs::HostPath &path)
{
   return path.name();
}

int main(int argc, char **argv)
{
   auto args = docopt::docopt(USAGE, { argv + 1, argv + argc }, true, "WiiU 0.1");
   bool result = false;

   if (args["--jitdebug"].asBool()) {
      gInterpreter.setJitMode(InterpJitMode::Debug);
   } else if (args["--jit"].asBool()) {
      gInterpreter.setJitMode(InterpJitMode::Enabled);
   } else {
      gInterpreter.setJitMode(InterpJitMode::Disabled);
   }

   // Create the logger
   std::vector<spdlog::sink_ptr> sinks;
   sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());

   if (args["--logfile"].asBool()) {
      std::string file;

      if (args["play"].asBool()) {
         file = getGameName(args["<game directory>"].asString());
      } else if (args["test"].asBool()) {
         file = "tests";
      }

      sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>(file, "txt", 23, 59, true));
   }

   if (args["--log-async"].asBool()) {
      spdlog::set_async_mode(0x100000);
   }

   gLog = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
   gLog->set_level(spdlog::level::trace);

   auto log_level = args["--log-level"].isString()? args["--log-level"].asString(): "trace";
   for (int l = spdlog::level::trace; l <= spdlog::level::off; l++) {
      if (spdlog::level::to_str((spdlog::level::level_enum) l) == log_level) {
         gLog->set_level((spdlog::level::level_enum) l);
         break;
      }
   }

   initialiseEmulator();

   if (args["play"].asBool()) {
      gLog->set_pattern("[%l:%t] %v");
      result = play(args["<game directory>"].asString());
   } else if (args["test"].asBool()) {
      gLog->set_pattern("%v");
      result = test(args["--as"].asString(), args["<test directory>"].asString());
   }

   system("PAUSE");
   return result ? 0 : -1;
}

static void
initialiseEmulator()
{
   platform::ui::initialise();

   Interpreter::RegisterFunctions();
   JitManager::RegisterFunctions();

   // Verify JIT and Interpreter have everything registered...
   for (size_t i = 0; i < static_cast<size_t>(InstructionID::InstructionCount); ++i) {
      if ((Interpreter::hasInstruction(static_cast<InstructionID>(i)) ^ 
         JitManager::hasInstruction(static_cast<InstructionID>(i))) != 0) {
         __debugbreak();
      }
   }
   
   // Kernel modules
   GameLoader::RegisterFunctions();
   CoreInit::RegisterFunctions();
   ErrEula::RegisterFunctions();
   GX2::RegisterFunctions();
   NNAc::RegisterFunctions();
   NNAct::RegisterFunctions();
   NNFp::RegisterFunctions();
   NNSave::RegisterFunctions();
   PadScore::RegisterFunctions();
   ProcUI::RegisterFunctions();
   SysApp::RegisterFunctions();
   Zlib125::RegisterFunctions();

   // Initialise emulator systems
   gMemory.initialise();
   gSystem.initialise();
   gInstructionTable.initialise();
   gJitManager.initialise();
   gSystem.registerModule("gameloader.rpl", new GameLoader{});
   gSystem.registerModule("coreinit.rpl", new CoreInit {});
   gSystem.registerModule("gx2.rpl", new GX2 {});
   gSystem.registerModule("nn_ac.rpl", new NNAc {});
   gSystem.registerModule("nn_act.rpl", new NNAct {});
   gSystem.registerModule("nn_fp.rpl", new NNFp {});
   gSystem.registerModule("nn_save.rpl", new NNSave {});
   gSystem.registerModule("nn_temp.rpl", new NNTemp{});
   gSystem.registerModule("proc_ui.rpl", new ProcUI {});
   gSystem.registerModule("zlib125.rpl", new Zlib125 {});
   gSystem.registerModule("padscore.rpl", new PadScore{});
   gSystem.registerModule("erreula.rpl", new ErrEula{});
   gSystem.registerModule("sysapp.rpl", new SysApp {});

   // Initialise debugger
   gDebugger.initialise();
}

static bool
test(const std::string &as, const std::string &path)
{
   return executeCodeTests(as, path);
}

static bool
play(const fs::HostPath &path)
{
   // Setup filesystem
   fs::FileSystem fs;
   fs.mountHostFolder("/vol", path.join("data"));
   gSystem.setFileSystem(&fs);

   // Read cos.xml
   pugi::xml_document doc;
   auto fh = fs.openFile("/vol/code/cos.xml", fs::File::Read);

   if (!fh) {
      gLog->error("Error opening /vol/code/cos.xml");
      return false;
   }

   auto size = fh->size();
   auto buffer = std::vector<char>(size);
   fh->read(buffer.data(), size);
   fh->close();

   // Parse cos.xml
   auto parseResult = doc.load_buffer_inplace(buffer.data(), buffer.size());

   if (!parseResult) {
      gLog->error("Error parsing /vol/code/cos.xml");
      return false;
   }

   auto app = doc.child("app");
   auto rpx = std::string { app.child("argstr").child_value() };
   auto maxCodeSize = std::stoul(app.child("max_codesize").child_value(), 0, 16);

   // Set up stuff..
   gLoader.initialise(maxCodeSize);

   // System preloaded modules
   gLoader.loadRPL("gameloader");
   gLoader.loadRPL("coreinit");

   // Startup processor
   gProcessor.start();

   // Start the loader
   {
      GameLoaderInit(rpx.c_str());

      auto thread = OSAllocFromSystem<OSThread>();
      auto stackSize = 2048;
      auto stack = reinterpret_cast<uint8_t*>(OSAllocFromSystem(stackSize, 8));
      auto name = OSSprintfFromSystem("Loader Thread");

      auto gameLoader = gLoader.loadRPL("gameloader");
      auto gameLoaderRun = gameLoader->findExport("GameLoaderRun");

      OSCreateThread(thread, 0, 0, nullptr,
                     stack + stackSize, stackSize, -2,
                     static_cast<OSThreadAttributes::Flags>(1 << 1));
      OSSetThreadName(thread, name);
      OSRunThread(thread, gameLoaderRun, 0, nullptr);
   }

   platform::ui::run();

   // Force inclusion in release builds
   tracePrint(nullptr, 0, 0);

   // Wait for all processor threads to exit
   //gProcessor.join();

   // TODO: OSFreeToSystem data
   return true;
}
