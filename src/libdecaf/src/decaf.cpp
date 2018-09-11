#include "decaf.h"
#include "decaf_config.h"
#include "decaf_graphics.h"
#include "decaf_input.h"
#include "decaf_slc.h"
#include "decaf_sound.h"
#include "debugger/debugger.h"
#include "debugger/ui/debugger_ui.h"
#include "filesystem/filesystem.h"
#include "input/input.h"
#include "ios/ios.h"
#include "kernel/kernel_filesystem.h"
#include "libcpu/cpu.h"
#include "libcpu/mem.h"

#include "cafe/kernel/cafe_kernel.h"
#include "cafe/kernel/cafe_kernel_process.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"
#include "cafe/libraries/swkbd/swkbd_keyboard.h"

#include <chrono>
#include <common/platform.h>
#include <common/platform_dir.h>
#include <condition_variable>
#include <fmt/format.h>
#include <mutex>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#ifdef PLATFORM_WINDOWS
#include <WinSock2.h>
#endif

namespace decaf
{

static ClipboardTextGetCallback
sClipboardTextGetCallbackFn = nullptr;

static ClipboardTextSetCallback
sClipboardTextSetCallbackFn = nullptr;

class LogFormatter : public spdlog::formatter
{
public:
   virtual void format(const spdlog::details::log_msg &msg, fmt::memory_buffer &dest) override
   {
      auto tm_time = spdlog::details::os::localtime(spdlog::log_clock::to_time_t(msg.time));
      auto duration = msg.time.time_since_epoch();
      auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000000;

      fmt::format_to(dest, "[{:02}:{:02}:{:02}.{:06} {}:",
                     tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, micros,
                     spdlog::level::to_c_str(msg.level));

      auto core = cpu::this_core::state();
      if (core) {
         auto thread = cafe::coreinit::internal::getCurrentThread();
         if (thread) {
            fmt::format_to(dest, "p{:01X} t{:02X}", core->id, thread->id);
         } else {
            fmt::format_to(dest, "p{:01X} t{:02X}", core->id, 0xFF);
         }
      } else if (msg.logger_name) {
         fmt::format_to(dest, "{}", *msg.logger_name);
      } else {
         fmt::format_to(dest, "h{}", msg.thread_id);
      }

      fmt::format_to(dest, "] {}{}",
                     std::string_view { msg.raw.data(), msg.raw.size() },
                     spdlog::details::os::default_eol);
   }

   virtual std::unique_ptr<formatter> clone() const override
   {
      return std::make_unique<LogFormatter>();
   }
};

std::string
makeConfigPath(const std::string &filename)
{
   return fs::HostPath { platform::getConfigDirectory() }
      .join("decaf")
      .join(filename)
      .path();
}

bool
createConfigDirectory()
{
   return platform::createParentDirectories(makeConfigPath("."));
}

void
initialiseLogging(const std::string &filename)
{
   std::vector<spdlog::sink_ptr> sinks;

   if (decaf::config::log::to_stdout) {
      sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
   }

   if (decaf::config::log::to_file) {
      auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      auto time = std::localtime(&now);

      auto logFilename =
         fmt::format("{}_{}-{:02}-{:02}_{:02}-{:02}-{:02}.txt",
                     filename,
                     time->tm_year + 1900, time->tm_mon, time->tm_mday,
                     time->tm_hour, time->tm_min, time->tm_sec);

      auto path = fs::HostPath { decaf::config::log::directory }.join(logFilename);
      sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.path()));
   }

   auto logLevel = spdlog::level::from_str(decaf::config::log::level);

   if (decaf::config::log::async) {
      spdlog::init_thread_pool(1024, 1);
      gLog = std::make_shared<spdlog::async_logger>("decaf",
                                                    begin(sinks), end(sinks),
                                                    spdlog::thread_pool());
   } else {
      gLog = std::make_shared<spdlog::logger>("decaf",
                                              begin(sinks), end(sinks));
      gLog->flush_on(spdlog::level::trace);
   }

   gLog->set_level(logLevel);
   gLog->set_formatter(std::make_unique<LogFormatter>());
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

#ifdef PLATFORM_WINDOWS
   WSADATA wsaInitData;
   WSAStartup(MAKEWORD(2, 2), &wsaInitData);
#endif

   // Initialise cpu (because this initialises memory)
   ::cpu::initialise();

   // Setup debugger
   debugger::initialise(makeConfigPath("imgui.ini"),
                        sClipboardTextGetCallbackFn,
                        sClipboardTextSetCallbackFn);

   // Setup filesystem
   auto filesystem = std::make_unique<fs::FileSystem>();

   // Find a valid application to run
   auto path = fs::HostPath { gamePath };
   auto volPath = fs::HostPath { };
   auto rpxPath = fs::HostPath { };

   if (platform::isDirectory(path.path())) {
      if (platform::isFile(path.join("code").join("cos.xml").path())) {
         // Found path/code/cos.xml
         volPath = path.path();
      } else if (platform::isFile(path.join("data").join("code").join("cos.xml").path())) {
         // Found path/data/code/cos.xml
         volPath = path.join("data").path();
      }
   } else if (platform::isFile(path.path())) {
      auto parent1 = fs::HostPath { path.parentPath() };
      auto parent2 = fs::HostPath { parent1.parentPath() };

      if (platform::isFile(parent2.join("code").join("cos.xml").path())) {
         // Found file/../code/cos.xml
         volPath = parent2.path();
      } else if (path.extension().compare("rpx") == 0) {
         // Found file.rpx
         rpxPath = path.path();
      }
   }

   if (!volPath.path().empty()) {
      filesystem->mountHostFolder("/vol/code", volPath.join("code"), fs::Permissions::Read);
      filesystem->mountHostFolder("/vol/content", volPath.join("content"), fs::Permissions::Read);
      filesystem->mountHostFolder("/vol/meta", volPath.join("meta"), fs::Permissions::Read);
   } else if (!rpxPath.path().empty()) {
      auto volCodePath = rpxPath.parentPath();
      filesystem->mountHostFolder("/vol/code", volCodePath, fs::Permissions::Read);

      if (!decaf::config::system::content_path.empty()) {
         filesystem->mountHostFolder("/vol/content", decaf::config::system::content_path, fs::Permissions::Read);
      }

      cafe::kernel::setExecutableFilename(rpxPath.filename());
   } else {
      gLog->error("Could not find valid application at {}", path.path());
      return false;
   }

   // Ensure mlc_path and slc_path exists on host file system
   if (!decaf::config::system::mlc_path.empty()) {
      platform::createDirectory(decaf::config::system::mlc_path);
   }

   if (!decaf::config::system::slc_path.empty()) {
      platform::createDirectory(decaf::config::system::slc_path);
   }

   if (!decaf::config::system::sdcard_path.empty()) {
      platform::createDirectory(decaf::config::system::sdcard_path);
   }

   // Add device folder
   filesystem->makeFolder("/dev");

   // Mount mlc device
   auto mlcPath = fs::HostPath { decaf::config::system::mlc_path };
   filesystem->mountHostFolder("/dev/mlc01", mlcPath, fs::Permissions::ReadWrite);

   // Mount slc device
   auto slcPath = fs::HostPath { decaf::config::system::slc_path };
   filesystem->mountHostFolder("/dev/slc01", slcPath, fs::Permissions::ReadWrite);

   // Initialise file system with necessary files
   internal::initialiseSlc(decaf::config::system::slc_path);

   // Setup ios
   kernel::setFileSystem(filesystem.get());
   ios::setFileSystem(std::move(filesystem));
   return true;
}

void
start()
{
   // Start ios
   ios::start();
}

bool
hasExited()
{
   return cafe::kernel::hasExited();
}

int
waitForExit()
{
   // Wait for IOS to finish
   ios::join();

   // Wait for PPC to finish
   cafe::kernel::join();

   // Make sure we clean up
   decaf::shutdown();

   return cafe::kernel::getProcessExitCode(cafe::kernel::RamPartitionId::MainApplication);
}

void
shutdown()
{
   // Shut down debugger
   debugger::shutdown();

   // Wait for IOS to finish
   ios::join();

   // Wait for PPC to finish
   cafe::kernel::join();

   // Stop graphics driver
   auto graphicsDriver = getGraphicsDriver();

   if (graphicsDriver) {
      graphicsDriver->stop();
   }

   setGraphicsDriver(nullptr);

   // Stop sound driver
   auto soundDriver = getSoundDriver();

   if (soundDriver) {
      soundDriver->stop();
   }

   setSoundDriver(nullptr);
}

void
injectMouseButtonInput(input::MouseButton button,
                       input::MouseAction action)
{
   debugger::ui::onMouseAction(button, action);
}

void
injectMousePos(float x,
               float y)
{
   debugger::ui::onMouseMove(x, y);
}

void
injectScrollInput(float xoffset,
                  float yoffset)
{
   debugger::ui::onMouseScroll(xoffset, yoffset);
}

void
injectKeyInput(input::KeyboardKey key,
               input::KeyboardAction action)
{
   if (!debugger::ui::onKeyAction(key, action)) {
      cafe::swkbd::internal::injectKeyInput(key, action);
   }
}

void
injectTextInput(const char *text)
{
   if (!debugger::ui::onText(text)) {
      cafe::swkbd::internal::injectTextInput(text);
   }
}

void
setClipboardTextCallbacks(ClipboardTextGetCallback getter,
                          ClipboardTextSetCallback setter)
{
   sClipboardTextGetCallbackFn = getter;
   sClipboardTextSetCallbackFn = setter;
}

} // namespace decaf
