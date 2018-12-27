#include "decaf.h"
#include "decaf_config.h"
#include "decaf_graphics.h"
#include "decaf_input.h"
#include "decaf_slc.h"
#include "decaf_sound.h"
#include "debugger/debugger.h"
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
#include <curl/curl.h>
#include <fmt/format.h>
#include <mutex>

#ifdef PLATFORM_WINDOWS
#include <WinSock2.h>
#endif

namespace decaf
{

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

   // Ensure paths exist
   if (!decaf::config::system::hfio_path.empty()) {
      platform::createDirectory(decaf::config::system::hfio_path);
   }

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

   if (!decaf::config::system::hfio_path.empty()) {
      // Optionally mount hfio device if path is set
      auto hfioPath = fs::HostPath { decaf::config::system::hfio_path };
      filesystem->mountHostFolder("/dev/hfio01", hfioPath, fs::Permissions::ReadWrite);
   }

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

} // namespace decaf
