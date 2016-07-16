#include "common/platform_dir.h"
#include "decaf.h"
#include "decaf_config.h"
#include "decaf_graphics.h"
#include "decaf_input.h"
#include "debugger/debugger.h"
#include "debugger/debugger_ui.h"
#include "filesystem/filesystem.h"
#include "input/input.h"
#include "kernel/kernel.h"
#include "kernel/kernel_hlefunction.h"
#include "kernel/kernel_filesystem.h"
#include "libcpu/cpu.h"
#include "libcpu/mem.h"
#include "modules/coreinit/coreinit_fs.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include <condition_variable>
#include <mutex>

namespace decaf
{

class LogFormatter : public spdlog::formatter
{
public:
   void format(spdlog::details::log_msg& msg) override
   {
      msg.formatted << '[';

      msg.formatted << spdlog::level::to_str(msg.level);

      msg.formatted << ':';

      auto core = cpu::this_core::state();
      if (core) {
         auto thread = coreinit::internal::getCurrentThread();
         if (thread) {
            msg.formatted.write("w{:01X}{:02X}", core->id, static_cast<uint16_t>(thread->id));
         } else {
            msg.formatted.write("w{:01X}{:02X}", core->id, 0xFF);
         }
      } else {
         msg.formatted << msg.thread_id;
      }

      msg.formatted << "] ";

      msg.formatted << fmt::StringRef(msg.raw.data(), msg.raw.size());
      msg.formatted.write(spdlog::details::os::eol, spdlog::details::os::eol_size);
   }
};

std::string
makeConfigPath(const std::string &filename)
{
   return fmt::format("{}/decaf/{}", platform::getConfigDirectory(), filename.c_str());
}

bool
createConfigDirectory()
{
   return platform::createParentDirectories(makeConfigPath("."));
}

void
initialiseLogging(std::vector<spdlog::sink_ptr> &sinks,
                  spdlog::level::level_enum level)
{
   gLog = std::make_shared<spdlog::logger>("decaf", begin(sinks), end(sinks));
   gLog->set_level(level);
   gLog->set_formatter(std::make_shared<LogFormatter>());
}

bool
initialise(const std::string &gamePath)
{
   if (!getInputDriver()) {
      gLog->error("No input driver set");
      return false;
   }

   if (!getGraphicsDriver()) {
      gLog->error("No graphics driver set");
      return false;
   }

   // Set JIT mode
   if (decaf::config::jit::enabled) {
      if (!decaf::config::jit::debug) {
         cpu::setJitMode(cpu::jit_mode::enabled);
      } else {
         cpu::setJitMode(cpu::jit_mode::debug);
      }
   } else {
      cpu::setJitMode(cpu::jit_mode::disabled);
   }

   // Setup core
   mem::initialise();
   cpu::initialise();
   kernel::initialise();

   // Setup filesystem
   auto filesystem = new fs::FileSystem();
   auto path = fs::HostPath { gamePath };

   if (platform::isDirectory(path.path())) {
      // See if we can find path/cos.xml
      filesystem->mountHostFolder("/vol", path);
      auto fh = filesystem->openFile("/vol/code/cos.xml", fs::File::Read);

      if (fh) {
         fh->close();
         delete fh;
      } else {
         // Try path/data
         filesystem->deleteFolder("/vol");
         filesystem->mountHostFolder("/vol", path.join("data"));
      }
   } else if (platform::isFile(path.path())) {
      // Load game file, currently only .rpx is supported
      // TODO: Support .WUD .WUX
      if (path.extension().compare("rpx") == 0) {
         filesystem->mountHostFile("/vol/code/" + path.filename(), path);
      } else {
         gLog->error("Only loading files with .rpx extension is currently supported {}", path.path());
         return false;
      }
   } else {
      gLog->error("Could not find file or directory {}", path.path());
      return false;
   }

   // Setup kernel
   kernel::setFileSystem(filesystem);
   kernel::setGameName(path.filename());

   // Mount system path
   auto systemPath = fs::HostPath { decaf::config::system::system_path };
   filesystem->mountHostFolder("/vol/storage_mlc01", systemPath.join("mlc"));

   // Startup the filesystem thread
   coreinit::internal::startFsThread();

   return true;
}

void
start()
{
   cpu::start();

   volatile int zero = 0;
   if (zero) {
      tracePrint(nullptr, 0, 0);
      traceReg(nullptr, 0, 0);
      traceRegStart(nullptr, 0, 0);
      traceRegNext(0);
      traceRegContinue();
   }
}

bool
hasExited()
{
   return kernel::hasExited();
}

int
waitForExit()
{
   // Wait for CPU to finish
   cpu::join();

   // Make sure we clean up
   decaf::shutdown();

   return kernel::getExitCode();
}

void
shutdown()
{
   // Make sure the CPU stops being blocked by the debugger.
   if (::debugger::paused()) {
      // TODO: This is technically a race as the debugger is designed such that
      //  it controls when the cores resume.  This might need to be routed through
      //  debugger::ui for that reason.
      ::debugger::resumeAll();
   }

   // Shut down CPU
   cpu::halt();

   // Wait for CPU to finish
   cpu::join();

   // Stop the FS
   coreinit::internal::shutdownFsThread();

   // Stop graphics driver
   auto graphicsDriver = getGraphicsDriver();

   if (graphicsDriver) {
      graphicsDriver->stop();
   }

   setGraphicsDriver(nullptr);
}

void
injectMouseButtonInput(input::MouseButton button,
                       input::MouseAction action)
{
   ::debugger::ui::injectMouseButtonInput(button, action);
}

void
injectMousePos(float x,
               float y)
{
   ::debugger::ui::injectMousePos(x, y);
}

void
injectScrollInput(float xoffset,
                  float yoffset)
{
   ::debugger::ui::injectScrollInput(xoffset, yoffset);
}

void
injectKeyInput(input::KeyboardKey key,
               input::KeyboardAction action)
{
   ::debugger::ui::injectKeyInput(key, action);
}

void
injectTextInput(const char *text)
{
   ::debugger::ui::injectTextInput(text);
}

void
setClipboardTextCallbacks(ClipboardTextGetCallback getter,
                          ClipboardTextSetCallback setter)
{
   ::debugger::ui::setClipboardTextCallbacks(getter, setter);
}

} // namespace decaf
