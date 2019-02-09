#include "decaf.h"
#include "decaf_config.h"
#include "decaf_graphics.h"
#include "decaf_input.h"
#include "decaf_slc.h"
#include "decaf_sound.h"

#include "cafe/kernel/cafe_kernel.h"
#include "cafe/kernel/cafe_kernel_process.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"
#include "cafe/libraries/swkbd/swkbd_keyboard.h"
#include "debugger/debugger.h"
#include "vfs/vfs_host_device.h"
#include "vfs/vfs_virtual_device.h"
#include "input/input.h"
#include "ios/ios.h"

#include <chrono>
#include <common/decaf_assert.h>
#include <common/platform.h>
#include <common/platform_dir.h>
#include <condition_variable>
#include <curl/curl.h>
#include <filesystem>
#include <fmt/format.h>
#include <libcpu/cpu.h>
#include <libcpu/mem.h>
#include <mutex>

#ifdef PLATFORM_WINDOWS
#include <WinSock2.h>
#endif

namespace decaf
{

std::string
makeConfigPath(const std::string &filename)
{
   auto configPath = std::filesystem::path { platform::getConfigDirectory() };
   return (configPath / "decaf" / filename).string();
}

bool
createConfigDirectory()
{
   return platform::createParentDirectories(makeConfigPath("."));
}

std::string
getResourcePath(const std::string &filename)
{
#ifdef PLATFORM_WINDOWS
   return decaf::config()->system.resources_path + "/" + filename;
#else
   std::string userPath = decaf::config()->system.resources_path + "/" + filename;
   std::string systemPath = std::string(DECAF_INSTALL_RESOURCESDIR) + "/" + filename;

   if (platform::fileExists(userPath)) {
      return userPath;
   }

   if (platform::fileExists(systemPath)) {
      return systemPath;
   }

   decaf_abort(fmt::format("Failed to find resource {}", filename));
#endif
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

   if (auto result = curl_global_init(CURL_GLOBAL_ALL); result != CURLE_OK) {
      gLog->error("curl_global_init returned {}", result);
   }

   // Initialise cpu (because this initialises memory)
   ::cpu::initialise();

   // Setup debugger
   debugger::initialise();

   // Setup filesystem
   auto filesystem = std::make_shared<vfs::VirtualDevice>("/");
   auto user = vfs::User { 0, 0 };

   // Find a valid application to run
   auto path = std::filesystem::path { gamePath };
   auto volPath = std::filesystem::path { };
   auto rpxPath = std::filesystem::path { };

   if (std::filesystem::is_directory(path)) {
      if (std::filesystem::is_regular_file(path / "code" / "cos.xml")) {
         // Found path/code/cos.xml
         volPath = path;
      } else if (std::filesystem::is_regular_file(path / "data" / "code" / "cos.xml")) {
         // Found path/data/code/cos.xml
         volPath = path / "data";
      }
   } else if (std::filesystem::is_regular_file(path)) {
      auto parent1 = path.parent_path();
      auto parent2 = parent1.parent_path();

      if (std::filesystem::is_regular_file(parent2 / "code" / "cos.xml")) {
         // Found file/../code/cos.xml
         volPath = parent2;
      } else if (path.extension().compare(".rpx") == 0) {
         // Found file.rpx
         rpxPath = path;
      }
   }

   // Initialise /vol
   filesystem->makeFolder(user, "/vol");
   filesystem->makeFolder(user, "/vol/sys");
   filesystem->makeFolder(user, "/vol/temp");

   if (!volPath.empty()) {
      filesystem->mountDevice(user, "/vol/code", std::make_shared<vfs::HostDevice>(volPath / "code"));
      filesystem->mountDevice(user, "/vol/content", std::make_shared<vfs::HostDevice>(volPath / "content"));
      filesystem->mountDevice(user, "/vol/meta", std::make_shared<vfs::HostDevice>(volPath / "meta"));
   } else if (!rpxPath.empty()) {
      filesystem->mountDevice(user, "/vol/code", std::make_shared<vfs::HostDevice>(rpxPath.parent_path()));

      if (!decaf::config()->system.content_path.empty()) {
         filesystem->mountDevice(user, "/vol/content", std::make_shared<vfs::HostDevice>(decaf::config()->system.content_path));
      }

      cafe::kernel::setExecutableFilename(rpxPath.filename().string());
   } else {
      gLog->error("Could not find valid application at {}", path.string());
      return false;
   }

   // Ensure paths exist
   if (!decaf::config()->system.hfio_path.empty()) {
      auto ec = std::error_code { };
      std::filesystem::create_directories(decaf::config()->system.hfio_path, ec);
   }

   if (!decaf::config()->system.mlc_path.empty()) {
      auto ec = std::error_code { };
      std::filesystem::create_directories(decaf::config()->system.mlc_path, ec);
   }

   if (!decaf::config()->system.slc_path.empty()) {
      auto ec = std::error_code { };
      std::filesystem::create_directories(decaf::config()->system.slc_path, ec);
   }

   if (!decaf::config()->system.sdcard_path.empty()) {
      auto ec = std::error_code { };
      std::filesystem::create_directories(decaf::config()->system.sdcard_path, ec);
   }

   // Add device folder
   filesystem->makeFolder(user, "/dev");

   // Mount devices
   filesystem->mountDevice(user, "/dev/mlc01",
                           std::make_shared<vfs::HostDevice>(decaf::config()->system.mlc_path));
   filesystem->mountDevice(user, "/dev/slc01",
                           std::make_shared<vfs::HostDevice>(decaf::config()->system.slc_path));
   filesystem->mountDevice(user, "/dev/ramdisk01",
                           std::make_shared<vfs::VirtualDevice>());

   if (!decaf::config()->system.hfio_path.empty()) {
      filesystem->mountDevice(user, "/dev/hfio01",
                              std::make_shared<vfs::HostDevice>(decaf::config()->system.hfio_path));
   }

   // Initialise file system with necessary files
   internal::initialiseSlc(decaf::config()->system.slc_path);

   // Setup ios
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

   // Stop IOS
   ios::stop();

   // Stop PPC
   cafe::kernel::stop();

   // Wait for IOS and PPC to stop
   ios::join();
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

} // namespace decaf
