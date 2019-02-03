#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include "test_helpers.h"
#include <spdlog/spdlog.h>

#ifdef DECAF_VULKAN
bool vulkanBeforeStart();
bool vulkanAfterComplete();
#else
bool vulkanBeforeStart() { return true;  }
bool vulkanAfterComplete() { return true; }
#endif

std::vector<uint8_t>
sRandomData = generateRandomData(32 * 1024 * 1024);

int main(int argc, char* argv[])
{
   // Set up a Vulkan instance
   if (!vulkanBeforeStart()) {
      printf("Could not initialize Vulkan\n");
      return -1;
   }

   // Run the test session
   int result = Catch::Session().run(argc, argv);

   // Shut down our Vulkan instance
   if (!vulkanAfterComplete()) {
      printf("Failed to shut down Vulkan\n");
   }

   return result;
}
