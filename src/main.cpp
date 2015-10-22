#include <pugixml.hpp>
#include <docopt.h>
#include "utils/bitutils.h"
#include "codetests.h"
#include "cpu/cpu.h"
#include "cpu/trace.h"
#include "debugger.h"
#include "fuzztests.h"
#include "filesystem/filesystem.h"
#include "hardwaretests.h"
#include "processor.h"
#include "loader.h"
#include "mem/mem.h"
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
#include "modules/nn_nfp/nn_nfp.h"
#include "modules/nn_save/nn_save.h"
#include "modules/nn_temp/nn_temp.h"
#include "modules/proc_ui/proc_ui.h"
#include "modules/padscore/padscore.h"
#include "modules/snd_core/snd_core.h"
#include "modules/swkbd/swkbd.h"
#include "modules/sysapp/sysapp.h"
#include "modules/vpad/vpad.h"
#include "modules/zlib125/zlib125.h"
#include "platform/platform_ui.h"
#include "system.h"
#include "usermodule.h"
#include "utils/log.h"
#include "utils/teenyheap.h"

std::shared_ptr<spdlog::logger>
gLog;

static void initialiseEmulator();
static bool test(const std::string &as, const std::string &path);
static bool fuzzTest();
static bool play(const fs::HostPath &path, const fs::HostPath &sysPath);

static const char USAGE[] =
R"(Decaf Emulator

Usage:
   decaf play [--jit | --jit-debug] [--log-file] [--log-async] [--log-level=<log-level>] [--sys-path=<sys-path>] <game directory>
   decaf test [--jit | --jit-debug] [--log-file] [--log-async] [--log-level=<log-level>] [--as=<ppcas>] <test directory>
   decaf fuzz
   decaf hwtest [--log-file]
   decaf (-h | --help)
   decaf --version

Options:
   -h --help     Show this screen.
   --version     Show version.
   --jit         Enables the JIT engine.
   --jit-debug   Verify JIT implementation against interpreter.
   --log-file    Redirect log output to file.
   --log-async   Enable asynchronous logging.
   --log-level=<log-level> [default: trace]
                 Only display logs with severity equal to or greater than this level.
                 Available levels: trace, debug, info, notice, warning, error, critical, alert, emerg, off
   --sys-path=<sys-path>
                 Where to locate any external system files.
   --as=<ppcas>  Path to PowerPC assembler [default: powerpc-eabi-as.exe].
)";

static const std::string&
getGameName(const fs::HostPath &path)
{
   return path.name();
}

int main(int argc, char **argv)
{
   auto args = docopt::docopt(USAGE, { argv + 1, argv + argc }, true, "Decaf 0.1");
   bool result = false;

   if (args["--jit-debug"].asBool()) {
      cpu::setJitMode(cpu::JitMode::Debug);
   } else if (args["--jit"].asBool()) {
      cpu::setJitMode(cpu::JitMode::Enabled);
   } else {
      cpu::setJitMode(cpu::JitMode::Disabled);
   }

   // Create the logger
   std::vector<spdlog::sink_ptr> sinks;
   sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());

   if (args["--log-file"].asBool()) {
      std::string file;

      if (args["play"].asBool()) {
         file = getGameName(args["<game directory>"].asString());
      } else if (args["test"].asBool()) {
         file = "tests";
      } else if (args["hwtest"].asBool()) {
         file = "hwtest";
      }

      sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>(file, "txt", 23, 59, true));
   }

   if (args["--log-async"].asBool()) {
      spdlog::set_async_mode(0x100000);
   }

   gLog = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
   gLog->set_level(spdlog::level::info);

   auto log_level = args["--log-level"].isString() ? args["--log-level"].asString() : "info";
   for (int l = spdlog::level::trace; l <= spdlog::level::off; l++) {
      if (spdlog::level::to_str((spdlog::level::level_enum) l) == log_level) {
         gLog->set_level((spdlog::level::level_enum) l);
         break;
      }
   }

   initialiseEmulator();

   if (args["play"].asBool()) {
      std::string sysPath = "/no_sys_specified";
      if (args["--sys-path"].isString()) {
         sysPath = args["--sys-path"].asString();
      }

      gLog->set_pattern("[%l:%t] %v");
      result = play(args["<game directory>"].asString(), sysPath);
   } else if (args["fuzz"].asBool()) {
      gLog->set_pattern("%v");
      result = fuzzTest();
   } else if (args["hwtest"].asBool()) {
      gLog->set_pattern("%v");
      result = hwtest::runTests();
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
   mem::initialise();
   cpu::initialise();

   // Kernel modules
   GameLoader::RegisterFunctions();
   CoreInit::RegisterFunctions();
   ErrEula::RegisterFunctions();
   GX2::RegisterFunctions();
   NN_ac::RegisterFunctions();
   NN_act::RegisterFunctions();
   NN_fp::RegisterFunctions();
   NN_nfp::RegisterFunctions();
   NN_save::RegisterFunctions();
   PadScore::RegisterFunctions();
   ProcUI::RegisterFunctions();
   Snd_Core::RegisterFunctions();
   Swkbd::RegisterFunctions();
   SysApp::RegisterFunctions();
   VPad::RegisterFunctions();
   Zlib125::RegisterFunctions();

   // Initialise emulator systems
   gSystem.initialise();
   gSystem.registerModule("gameloader.rpl", new GameLoader{});
   gSystem.registerModule("coreinit.rpl", new CoreInit {});
   gSystem.registerModule("erreula.rpl", new ErrEula {});
   gSystem.registerModule("gx2.rpl", new GX2 {});
   gSystem.registerModule("nn_ac.rpl", new NN_ac {});
   gSystem.registerModule("nn_act.rpl", new NN_act {});
   gSystem.registerModule("nn_fp.rpl", new NN_fp {});
   gSystem.registerModule("nn_nfp.rpl", new NN_nfp {});
   gSystem.registerModule("nn_save.rpl", new NN_save {});
   gSystem.registerModule("nn_temp.rpl", new NN_temp {});
   gSystem.registerModule("padscore.rpl", new PadScore {});
   gSystem.registerModule("proc_ui.rpl", new ProcUI {});
   gSystem.registerModule("snd_core.rpl", new Snd_Core {});
   gSystem.registerModule("swkbd.rpl", new Swkbd{});
   gSystem.registerModule("sysapp.rpl", new SysApp {});
   gSystem.registerModule("vpad.rpl", new VPad {});
   gSystem.registerModule("zlib125.rpl", new Zlib125 {});

   // Initialise debugger
   gDebugger.initialise();
}

static bool
test(const std::string &as, const std::string &path)
{
   return executeCodeTests(as, path);
}

static bool
fuzzTest()
{
   return executeFuzzTests();
}

static bool
play(const fs::HostPath &path, const fs::HostPath &sysPath)
{
   // Create window
   if (!platform::ui::createWindow(L"Decaf")) {
      gLog->error("Error creating window");
      return false;
   }

   // Setup filesystem
   fs::FileSystem fs;
   fs.mountHostFolder("/vol", path.join("data"));
   fs.mountHostFolder("/vol/storage_mlc01", sysPath.join("mlc"));
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

   // Allocate memory for the emulator
   mem::alloc(0x01000000, 0x01000000); // System
   mem::alloc(0x02000000, 0x40000000); // MEM2
   mem::alloc(0xe0000000, 0x02800000); // Foreground
   mem::alloc(0xf4000000, 0x02000000); // MEM1
   mem::alloc(0xf8000000, 0x00010000); // L2CACHE
   // Lock out some memory for unimplemented data access
   mem::protect(0xfff00000, 0x000fffff);

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
                     reinterpret_cast<be_val<uint32_t>*>(stack + stackSize), stackSize, -2,
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
