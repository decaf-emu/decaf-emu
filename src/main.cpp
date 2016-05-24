#include <pugixml.hpp>
#include <docopt.h>
#include "config.h"
#include "utils/bitutils.h"
#include "cpu/cpu.h"
#include "cpu/trace.h"
#include "cpu/jit/jit.h"
#include "debugger.h"
#include "fuzztests.h"
#include "filesystem/filesystem.h"
#include "gpu/opengl/opengl_driver.h"
#include "hardwaretests.h"
#include "input/input.h"
#include "processor.h"
#include "loader.h"
#include "mem/mem.h"
#include "modules/gameloader/gameloader.h"
#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/erreula/erreula.h"
#include "modules/gx2/gx2.h"
#include "modules/mic/mic.h"
#include "modules/nn_ac/nn_ac.h"
#include "modules/nn_acp/nn_acp.h"
#include "modules/nn_act/nn_act.h"
#include "modules/nn_boss/nn_boss.h"
#include "modules/nn_fp/nn_fp.h"
#include "modules/nn_ndm/nn_ndm.h"
#include "modules/nn_nfp/nn_nfp.h"
#include "modules/nn_olv/nn_olv.h"
#include "modules/nn_save/nn_save.h"
#include "modules/nn_temp/nn_temp.h"
#include "modules/nsysnet/nsysnet.h"
#include "modules/proc_ui/proc_ui.h"
#include "modules/padscore/padscore.h"
#include "modules/snd_core/snd_core.h"
#include "modules/swkbd/swkbd.h"
#include "modules/sysapp/sysapp.h"
#include "modules/vpad/vpad.h"
#include "modules/zlib125/zlib125.h"
#include "platform/platform_dir.h"
#include "platform/platform_glfw.h"
#include "platform/platform_sdl.h"
#include "platform/platform_ui.h"
#include "system.h"
#include "usermodule.h"
#include "utils/log.h"
#include "utils/teenyheap.h"

static void
initialiseEmulator();

static bool
play(const fs::HostPath &path);

static const char USAGE[] =
R"(Decaf Emulator

Usage:
   decaf play [--jit | --jit-debug] [--log-file] [--log-async] [--no-log-stdout] [--log-level=<log-level>] [--sys-path=<sys-path>] <game directory>
   decaf fuzz
   decaf hwtest [--log-file] [--jit]
   decaf (-h | --help)
   decaf --version

Options:
   -h --help     Show this screen.
   --version     Show version.
   --jit         Enables the JIT engine.
   --jit-debug   Verify JIT implementation against interpreter.
   --no-log-stdout
                 Disable logging to stdout
   --log-file    Redirect log output to file.
   --log-async   Enable asynchronous logging.
   --log-level=<log-level> [default: trace]
                 Only display logs with severity equal to or greater than this level.
                 Available levels: trace, debug, info, notice, warning, error, critical, alert, emerg, off
   --sys-path=<sys-path> 
                 Where to locate any external system files.
)";

static const std::string
getGameName(const fs::HostPath &path)
{
   return path.filename();
}

int main(int argc, char **argv)
{
   auto args = docopt::docopt(USAGE, { argv + 1, argv + argc }, true, "Decaf 0.1");
   auto has_arg = [&args](const char *name) { return args.find(name) != args.end() ? (bool)args[name] : false; };
   auto arg_bool = [&](const char *name) { return has_arg(name) && args[name].isBool() ? args[name].asBool() : false; };
   auto arg_str = [&](const char *name) { return has_arg(name) && args[name].isString() ? args[name].asString() : ""; };
   auto result = false;

   // First thing, load the config!
   config::load("config.json");

   // Allow command line options to override config
   if (arg_bool("--jit-debug")) {
      config::jit::debug = true;
   } else if (arg_bool("--jit")) {
      config::jit::enabled = true;
   }

   if (arg_bool("--no-log-stdout")) {
      config::log::to_stdout = true;
   }

   if (arg_bool("--log-file")) {
      config::log::to_file = true;
   }

   if (arg_bool("--log-async")) {
      config::log::async = true;
   }

   if (has_arg("--log-level")) {
      config::log::level = arg_str("--log-level");
   }

   if (has_arg("--sys-path")) {
      config::system::system_path = arg_str("--sys-path");
   }

   // Set log filename
   std::string logFilename;

   if (arg_bool("play")) {
      logFilename = getGameName(arg_str("<game directory>"));
   } else if (arg_bool("test")) {
      logFilename = "tests";
   } else if (arg_bool("hwtest")) {
      logFilename = "hwtest";
   } else {
      logFilename = "log";
   }

   // Start!
   logging::initialise(logFilename);

   if (arg_bool("play")) {
      gLog->set_pattern("[%l:%t] %v");
      result = play(args["<game directory>"].asString());
   } else if (arg_bool("fuzz")) {
      gLog->set_pattern("%v");
      result = executeFuzzTests();
   } else if (arg_bool("hwtest")) {
      gLog->set_pattern("%v");
      result = hwtest::runTests("tests/cpu/wiiu");
   }

#ifdef PLATFORM_WINDOWS
   system("PAUSE");
#endif
   return result ? 0 : -1;
}

static void
initialiseEmulator()
{
   // Setup jit from config
   if (config::jit::enabled) {
      if (config::jit::debug) {
         cpu::setJitMode(cpu::JitMode::Debug);
      } else {
         cpu::setJitMode(cpu::JitMode::Enabled);
      }
   } else {
      cpu::setJitMode(cpu::JitMode::Disabled);
   }

   // Setup core
   mem::initialise();
   cpu::initialise();

   // Kernel modules
   GameLoader::RegisterFunctions();
   coreinit::Module::RegisterFunctions();
   nn::erreula::Module::RegisterFunctions();
   gx2::Module::RegisterFunctions();
   mic::Module::RegisterFunctions();
   nn::ac::Module::RegisterFunctions();
   nn::acp::Module::RegisterFunctions();
   nn::act::Module::RegisterFunctions();
   nn::boss::Module::RegisterFunctions();
   nn::fp::Module::RegisterFunctions();
   nn::ndm::Module::RegisterFunctions();
   nn::nfp::Module::RegisterFunctions();
   nn::olv::Module::RegisterFunctions();
   nn::save::Module::RegisterFunctions();
   nn::temp::Module::RegisterFunctions();
   nsysnet::Module::RegisterFunctions();
   padscore::Module::RegisterFunctions();
   proc_ui::Module::RegisterFunctions();
   snd_core::Module::RegisterFunctions();
   nn::swkbd::Module::RegisterFunctions();
   sysapp::Module::RegisterFunctions();
   vpad::Module::RegisterFunctions();
   zlib125::Module::RegisterFunctions();

   // Initialise emulator systems
   gSystem.initialise();
   gSystem.registerModule("gameloader.rpl", new GameLoader{});
   gSystem.registerModule("coreinit.rpl", new coreinit::Module {});
   gSystem.registerModule("erreula.rpl", new nn::erreula::Module {});
   gSystem.registerModule("gx2.rpl", new gx2::Module {});
   gSystem.registerModule("mic.rpl", new mic::Module {});
   gSystem.registerModule("nn_ac.rpl", new nn::ac::Module {});
   gSystem.registerModule("nn_acp.rpl", new nn::acp::Module {});
   gSystem.registerModule("nn_act.rpl", new nn::act::Module {});
   gSystem.registerModule("nn_boss.rpl", new nn::boss::Module {});
   gSystem.registerModule("nn_fp.rpl", new nn::fp::Module {});
   gSystem.registerModule("nn_nfp.rpl", new nn::nfp::Module {});
   gSystem.registerModule("nn_ndm.rpl", new nn::ndm::Module {});
   gSystem.registerModule("nn_olv.rpl", new nn::olv::Module {});
   gSystem.registerModule("nn_save.rpl", new nn::save::Module {});
   gSystem.registerModule("nn_temp.rpl", new nn::temp::Module {});
   gSystem.registerModule("nsysnet.rpl", new nsysnet::Module {});
   gSystem.registerModule("padscore.rpl", new padscore::Module {});
   gSystem.registerModule("proc_ui.rpl", new proc_ui::Module {});
   gSystem.registerModule("snd_core.rpl", new snd_core::Module {});
   gSystem.registerModuleAlias("snd_core.rpl", "sndcore2.rpl");
   gSystem.registerModule("swkbd.rpl", new nn::swkbd::Module {});
   gSystem.registerModule("sysapp.rpl", new sysapp::Module {});
   gSystem.registerModule("vpad.rpl", new vpad::Module {});
   gSystem.registerModule("zlib125.rpl", new zlib125::Module {});

   // Initialise debugger
   gDebugger.initialise();
}

static bool
play(const fs::HostPath &path)
{
   // Create gpu driver
   gpu::driver::create(new gpu::opengl::GLDriver);

   // Create window
#ifdef DECAF_GLFW
   if (config::system::platform.compare("glfw") == 0) {
      gLog->info("Using GLFW");
      platform::setPlatform(new platform::PlatformGLFW);
   } else
#endif
#ifdef DECAF_SDL
   if (config::system::platform.compare("sdl") == 0) {
      gLog->info("Using SDL");
      platform::setPlatform(new platform::PlatformSDL);
   } else
#endif
   {
      gLog->error("Unsupported platform {}");
      return false;
   }

   if (!platform::ui::init()) {
      gLog->error("Error initializing UI");
      return false;
   }

   if (!platform::ui::createWindows("Decaf", "Decaf - Gamepad")) {
      gLog->error("Error creating window");
      return false;
   }

   if (!input::init()) {
      gLog->error("Error setting up input");
      return false;
   }

   // Setup filesystem
   fs::FileSystem fs;
   fs::HostPath sysPath = config::system::system_path;

   if (platform::isDirectory(path.path())) {
      // Load game directory
      fs.mountHostFolder("/vol", path.join("data"));
   } else if (platform::isFile(path.path())) {
      // Load game file, currently only .rpx is supported
      // TODO: Support .WUD .WUX
      if (path.extension().compare("rpx") == 0) {
         fs.mountHostFile("/vol/code/" + path.filename(), path);
      } else {
         gLog->error("Only loading files with .rpx extension is currently supported {}", path.path());
         return false;
      }
   } else {
      gLog->error("Could not find file or directory {}", path.path());
      return false;
   }

   gSystem.setFileSystem(&fs);

   // Read cos.xml if found
   auto maxCodeSize = 0x0E000000u;
   auto rpx = path.filename();

   if (auto fh = fs.openFile("/vol/code/cos.xml", fs::File::Read)) {
      auto size = fh->size();
      auto buffer = std::vector<uint8_t>(size);
      fh->read(buffer.data(), size, 1);
      fh->close();

      // Parse cos.xml
      pugi::xml_document doc;
      auto parseResult = doc.load_buffer_inplace(buffer.data(), buffer.size());

      if (!parseResult) {
         gLog->error("Error parsing /vol/code/cos.xml");
         return false;
      }

      auto app = doc.child("app");
      rpx = std::string { app.child("argstr").child_value() };
      maxCodeSize = std::stoul(app.child("max_codesize").child_value(), 0, 16);
   } else {
      gLog->warn("Could not open /vol/code/cos.xml, using default values");
   }

   if (auto fh = fs.openFile("/vol/code/app.xml", fs::File::Read)) {
      auto size = fh->size();
      auto buffer = std::vector<uint8_t>(size);
      fh->read(buffer.data(), size, 1);
      fh->close();

      // Parse app.xml
      pugi::xml_document doc;
      auto parseResult = doc.load_buffer_inplace(buffer.data(), buffer.size());

      if (!parseResult) {
         gLog->error("Error parsing /vol/code/app.xml");
         return false;
      }

      // Set os_version and title_id
      auto app = doc.child("app");
      auto os_version = std::stoull(app.child("os_version").child_value(), 0, 16);
      auto title_id = std::stoull(app.child("title_id").child_value(), 0, 16);

      coreinit::internal::setSystemID(os_version);
      coreinit::internal::setTitleID(title_id);
   } else {
      gLog->warn("Could not open /vol/code/app.xml, using default values");
   }

   // Mount system path
   fs.mountHostFolder("/vol/storage_mlc01", sysPath.join("mlc"));

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
      using namespace coreinit;
      GameLoaderInit(rpx.c_str());

      auto thread = coreinit::internal::sysAlloc<OSThread>();
      auto stackSize = 2048;
      auto stack = reinterpret_cast<uint8_t *>(coreinit::internal::sysAlloc(stackSize, 8));
      auto name = coreinit::internal::sysStrDup("Loader Thread");

      auto gameLoader = gLoader.loadRPL("gameloader");
      auto gameLoaderRun = gameLoader->findExport("GameLoaderRun");

      OSCreateThread(thread, 0, 0, nullptr,
                     reinterpret_cast<be_val<uint32_t>*>(stack + stackSize), stackSize, -2,
                     OSThreadAttributes::AffinityCPU1);
      OSSetThreadName(thread, name);
      OSRunThread(thread, gameLoaderRun, 0, nullptr);
   }

   platform::ui::run();

   platform::ui::shutdown();
   config::save("config.json");

   // Force inclusion in release builds
   volatile int zero = 0;
   if (zero) {
      tracePrint(nullptr, 0, 0);
      tracePrintSyscall(0);
      fallbacksPrint();
   }

   // Stop all processor threads
   gProcessor.stop();

   // TODO: OSFreeToSystem data
   return true;
}
