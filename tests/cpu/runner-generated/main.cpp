#include "hardwaretests.h"

#include <common/decaf_assert.h>
#include <libcpu/cpu.h>
#include <libcpu/cpu_config.h>
#include <libcpu/mem.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

std::shared_ptr<spdlog::logger>
gLog;

static int runResult;

int main(int argc, char *argv[])
{
   gLog = std::make_shared<spdlog::logger>("logger", std::make_shared<spdlog::sinks::stdout_sink_st>());
   gLog->set_level(spdlog::level::debug);

   cpu::config::jit::enabled = true;
   cpu::initialise();

   // We need to run the tests on a core.
   cpu::setCoreEntrypointHandler(
      [](cpu::Core *core) {
         if (cpu::this_core::id() == 1) {
            // Run the tests on only a single core.
            runResult = hwtest::runTests("data/wiiu");
         }
      });

   cpu::start();
   cpu::join();
   return runResult;
}
