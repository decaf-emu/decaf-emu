#include "decaf.h"
#include "gpu/opengl/opengl_driver.h"
#include "input/input.h"
#include "filesystem/filesystem.h"
#include "cpu/mem.h"
#include "cpu/cpu.h"
#include "kernel/kernel.h"
#include "kernel/kernel_hlefunction.h"
#include "kernel/kernel_filesystem.h"
#include "debugger.h"
#include "platform/platform_dir.h"
#include <mutex>
#include <condition_variable>
#include "modules/coreinit/coreinit_scheduler.h"

namespace decaf
{

static std::string
gSystemPath = "";

static std::string
gGamePath = "";

static gpu::opengl::GLDriver *
gGlDriver;

void setSystemPath(const std::string &path)
{
   gSystemPath = path;
}

void setGamePath(const std::string &path)
{
   gGamePath = path;
}

void setKernelTraceEnabled(bool enabled)
{
   kernel::functions::enableTrace = enabled;
}

void setJitMode(bool enabled, bool debug)
{
   if (enabled) {
      if (!debug) {
         cpu::set_jit_mode(cpu::jit_mode::enabled);
      } else {
         cpu::set_jit_mode(cpu::jit_mode::debug);
      }
   } else {
      cpu::set_jit_mode(cpu::jit_mode::disabled);
   }
}

class LogFormatter : public spdlog::formatter
{
public:
   void format(spdlog::details::log_msg& msg) override {

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

void initLogging(std::vector<spdlog::sink_ptr> &sinks, spdlog::level::level_enum level)
{
   gLog = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
   gLog->set_level(level);
   gLog->set_formatter(std::make_shared<LogFormatter>());
}

bool initialise()
{
   // Setup core
   mem::initialise();
   cpu::initialise();
   kernel::initialise();

   // Initialise debugger
   gDebugger.initialise();

   gGlDriver = new gpu::opengl::GLDriver();

   // Initialise Input stuff...

   // Setup filesystem
   fs::FileSystem *fs = new fs::FileSystem();
   fs::HostPath path = gGamePath;
   fs::HostPath sysPath = gSystemPath;

   if (platform::isDirectory(path.path())) {
      // See if we can find path/cos.xml
      fs->mountHostFolder("/vol", path);
      auto fh = fs->openFile("/vol/code/cos.xml", fs::File::Read);

      if (fh) {
         fh->close();
         delete fh;
      } else {
         // Try path/data
         fs->deleteFolder("/vol");
         fs->mountHostFolder("/vol", path.join("data"));
      }
   } else if (platform::isFile(path.path())) {
      // Load game file, currently only .rpx is supported
      // TODO: Support .WUD .WUX
      if (path.extension().compare("rpx") == 0) {
         fs->mountHostFile("/vol/code/" + path.filename(), path);
      } else {
         gLog->error("Only loading files with .rpx extension is currently supported {}", path.path());
         return false;
      }
   } else {
      gLog->error("Could not find file or directory {}", path.path());
      return false;
   }

   kernel::setFileSystem(fs);
   kernel::set_game_name(path.filename());

   // Lock out some memory for unimplemented data access
   mem::protect(0xfff00000, 0x000fffff);

   // Mount system path
   fs->mountHostFolder("/vol/storage_mlc01", sysPath.join("mlc"));

   return true;
}

static std::mutex
sGpuMutex;

static std::condition_variable
sGpuCond;

static std::atomic_bool
gGpuRunning = false;

void start()
{
   cpu::start();
}

void shutdown()
{
   // Completely shut down the CPU (this waits for it to stop)
   cpu::halt();
}

void runGpuDriver()
{  
   std::unique_lock<std::mutex> lock(sGpuMutex);
   gGpuRunning.store(true);

   lock.unlock();
   gGlDriver->run();
   lock.lock();
   
   gGpuRunning.store(false);
   sGpuCond.notify_one();
}

void shutdownGpuDriver()
{
   // Alert the GPU that it needs to stop
   gGlDriver->stop();

   // Wait for the GPU to shut down
   std::unique_lock<std::mutex> lock(sGpuMutex);
   while (gGpuRunning.load()) {
      sGpuCond.wait(lock);
   }
}

void getSwapBuffers(gl::GLuint* tv, gl::GLuint* drc)
{
   gGlDriver->getSwapBuffers(tv, drc);
}

float getAverageFps()
{
   return gGlDriver->getAverageFps();
}

void setVpadCoreButtonCallback(input::ButtonStatus(*fn)(input::vpad::Channel channel, input::vpad::Core button))
{
   ::input::setVpadCoreButtonCallback(fn);
}

} // namespace decaf
