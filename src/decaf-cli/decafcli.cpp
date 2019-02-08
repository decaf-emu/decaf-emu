#include "decafcli.h"
#include "config.h"

#include <chrono>
#include <condition_variable>
#include <libgpu/gpu_graphicsdriver.h>
#include <libdecaf/decaf_nullinputdriver.h>
#include <mutex>
#include <thread>

int
DecafCLI::run(const std::string &gamePath)
{
   int result = 0;

   // Setup drivers
   decaf::setGraphicsDriver(gpu::createNullDriver());
   decaf::setInputDriver(new decaf::NullInputDriver());

   // Initialise emulator
   if (!decaf::initialise(gamePath)) {
      return -1;
   }

   // Start graphics thread
   auto graphicsThread = std::thread {
      [this]() {
         decaf::getGraphicsDriver()->run();
      } };

   // Setup timeout stuff
   std::atomic_bool running { true };
   std::atomic_bool timedOut { false };
   std::thread timeoutThread;
   std::condition_variable timeoutCV;

   if (config::system::timeout_ms) {
      auto timeout = std::chrono::system_clock::now() + std::chrono::milliseconds(config::system::timeout_ms);

      timeoutThread = std::thread {
         [&]() {
            std::mutex timeoutMutex;
            std::unique_lock<std::mutex> lock { timeoutMutex };
            timeoutCV.wait_until(lock, timeout, [&]() { return !running.load(); });

            if (running) {
               timedOut.store(true);
               decaf::shutdown();
            }
         } };
   }

   // Start emulator
   decaf::start();

   // Wait until program completes
   result = decaf::waitForExit();

   // If we didn't timeout, wakeup timeout thread
   if (!timedOut.load()) {
      running.store(false);
      timeoutCV.notify_all();
   } else {
      gCliLog->error("Application exceeded maxmimum execution time of {}ms.", config::system::timeout_ms);
      result = -1;
   }

   // Wait for timeout thread to exit
   if (timeoutThread.joinable()) {
      timeoutThread.join();
   }

   // Wait for the GPU thread to exit
   if (graphicsThread.joinable()) {
      graphicsThread.join();
   }

   return result;
}
