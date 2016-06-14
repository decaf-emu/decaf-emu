#include "common/platform_dir.h"
#include "decaf.h"
#include "decaf_config.h"
#include "debugger/debugger.h"
#include "debugger/debugger_ui.h"
#include "filesystem/filesystem.h"
#include "gpu/opengl/opengl_driver.h"
#include "input/input.h"
#include "kernel/kernel.h"
#include "kernel/kernel_hlefunction.h"
#include "kernel/kernel_filesystem.h"
#include "libcpu/cpu.h"
#include "libcpu/mem.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include <condition_variable>
#include <mutex>

namespace decaf
{

static GraphicsDriver *
sGraphicsDriver = nullptr;

static InputProvider *
sInputProvider = nullptr;

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
   if (!sInputProvider) {
      gLog->error("No input provider set");
      return false;
   }

   if (!sGraphicsDriver) {
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
   ::debugger::initialise();

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

   return true;
}

OpenGLDriver *
createGLDriver()
{
   return new gpu::opengl::GLDriver();
}

void
start()
{
   ::debugger::ui::initialise();
   cpu::start();
}

void
shutdown()
{
   // Completely shut down the CPU (this waits for it to stop)
   cpu::halt();
}

void
setGraphicsDriver(GraphicsDriver *driver)
{
   sGraphicsDriver = driver;
}

GraphicsDriver *
getGraphicsDriver()
{
   return sGraphicsDriver;
}

void
setInputProvider(InputProvider *provider)
{
   sInputProvider = provider;
}

InputProvider *
getInputProvider()
{
   return sInputProvider;
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
