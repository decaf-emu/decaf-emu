#include <common/decaf_assert.h>
#include <common/log.h>
#include <fstream>
#include <libcpu/cpu.h>
#include <libcpu/cpu_config.h>
#include <libcpu/espresso/espresso_disassembler.h>
#include <libcpu/mem.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

int
runTests()
{
   // Built From http://achurch.org/cpu-tests/ppc750cl.s
   // powerpc-eabi-as ppc750cl.S -o ppc750cl.o --defsym TEST_SC=0 --defsym HAVE_UGQR=1 --defsym TEST_TRAP=0
   // powerpc-eabi-ld ppc750cl.o -o ppc750cl --oformat=binary
   std::ifstream file { "data/achurch.bin", std::ifstream::in | std::ifstream::binary };

   if (!file.is_open()) {
      gLog->error("Could not open data/achurch.bin");
      return -1;
   }

   // Calculate the total size of the file
   file.seekg(0, std::ios::end);
   auto file_size = file.tellg();
   file.seekg(0, std::ios::beg);

   // Allocate code memory
   auto baseCodeAddress = cpu::VirtualAddress { 0x01000000u };
   auto baseCodePhysicalAddress = cpu::PhysicalAddress { 0x50000000u };
   auto codeSize = align_up(static_cast<uint32_t>(file_size), cpu::PageSize);
   cpu::allocateVirtualAddress(baseCodeAddress, codeSize);
   cpu::mapMemory(baseCodeAddress, baseCodePhysicalAddress, codeSize, cpu::MapPermission::ReadWrite);

   // Allocate data memory
   auto baseDataAddress = cpu::VirtualAddress { 0x03000000u };
   auto baseDataPhysicalAddress = cpu::PhysicalAddress { 0x52000000u };
   auto dataSize = 0x02000000u;
   cpu::allocateVirtualAddress(baseDataAddress, dataSize);
   cpu::mapMemory(baseDataAddress, baseDataPhysicalAddress, dataSize, cpu::MapPermission::ReadWrite);

   // Read the file directly into PPC memory
   file.read(mem::translate<char>(baseCodeAddress.getAddress()), file_size);

   auto core = cpu::this_core::state();
   auto scratchMemAddr = baseDataAddress.getAddress() + (dataSize / 2);
   auto failResultsAddr = baseDataAddress.getAddress();
   std::memset(mem::translate<void>(scratchMemAddr), 0, 32 * 1024u);

   core->nia = baseCodeAddress.getAddress();
   core->gpr[3] = 0;
   core->gpr[4] = scratchMemAddr;
   core->gpr[5] = failResultsAddr;
   core->fpr[1].paired0 = 1.0;
   cpu::this_core::executeSub();

   auto failedTests = core->gpr[3];

   if (failedTests) {
      for (uint32_t i = 0; i < failedTests; ++i) {
         uint32_t failedInstr = mem::read<uint32_t>(failResultsAddr + (i * 4 * 8) + 0);
         uint32_t failedAddr = mem::read<uint32_t>(failResultsAddr + (i * 4 * 8) + 4);
         uint32_t failedAux[] = {
            mem::read<uint32_t>(failResultsAddr + (i * 4 * 8) + 8),
            mem::read<uint32_t>(failResultsAddr + (i * 4 * 8) + 12),
            mem::read<uint32_t>(failResultsAddr + (i * 4 * 8) + 16),
            mem::read<uint32_t>(failResultsAddr + (i * 4 * 8) + 20)
         };

         espresso::Disassembly disassembly;
         espresso::disassemble(failedInstr, disassembly, failedAddr);
         gLog->warn(" {:08x} = {:08x} failed - {}", failedAddr, failedInstr, disassembly.text);
         gLog->warn("   {:08x} {:08x} {:08x} {:08x}", failedAux[0], failedAux[1], failedAux[2], failedAux[3]);
      }

      gLog->error("Failed {} tests.", failedTests);
      return static_cast<int>(failedTests);
   }

   return 0;
}

int main(int argc, char *argv[])
{
   int runResult;
   auto logger = std::make_shared<spdlog::logger>("logger", std::make_shared<spdlog::sinks::stdout_sink_st>());
   logger->set_level(spdlog::level::debug);
   gLog = logger;

   auto cpuConfig = cpu::Settings { };
   cpuConfig.jit.enabled = true;
   cpu::setConfig(cpuConfig);
   cpu::initialise();

   // We need to run the tests on a core.
   cpu::setCoreEntrypointHandler(
      [&runResult](cpu::Core *core) {
         if (cpu::this_core::id() == 1) {
            // Run the tests on only a single core.
            runResult = runTests();
         }
      });

   cpu::start();
   cpu::join();
   return runResult;
}
