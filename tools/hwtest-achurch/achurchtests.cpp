#include "achurchtests.h"
#include "common/log.h"
#include "libcpu/cpu.h"
#include "libcpu/espresso/espresso_disassembler.h"
#include "libcpu/mem.h"
#include <fstream>

namespace actest
{

bool
runTests()
{
   // Built From http://achurch.org/cpu-tests/ppc750cl.s
   //   ESPRESSO = 1
   //   TEST_BA = 1
   //   TEST_SC = 0
   //   TEST_TRAP = 0
   //   TEST_PAIRED_SINGLE = 1
   // powerpc-eabi-as ppc750cl.S -o ppc750cl.o
   // powerpc-eabi-ld ppc750cl.o -o ppc750cl --oformat=binary
   std::ifstream file("tests/cpu/achurch.bin", std::ifstream::in | std::ifstream::binary);

   // Calculate the total size of the file
   file.seekg(0, std::ios::end);
   auto file_size = file.tellg();
   file.seekg(0, std::ios::beg);

   // Read the file directly into PPC memory
   file.read(mem::translate<char>(0x01000000), file_size);

   auto core = cpu::this_core::state();

   core->nia = 0x01000000;
   core->gpr[3] = 0;
   core->gpr[4] = 0x04000000;
   core->gpr[5] = 0x02000000;
   core->fpr[1].paired0 = 1.0;
   cpu::this_core::executeSub();

   auto failedTests = core->gpr[3];
   gLog->info("Test run completed, {} tests failed", failedTests);

   for (uint32_t i = 0; i < failedTests; ++i) {
      uint32_t failedInstr = mem::read<uint32_t>(0x02000000 + (i * 4 * 8) + 0);
      uint32_t failedAddr = mem::read<uint32_t>(0x02000000 + (i * 4 * 8) + 4);
      uint32_t failedAux[] = {
         mem::read<uint32_t>(0x02000000 + (i * 4 * 8) + 8),
         mem::read<uint32_t>(0x02000000 + (i * 4 * 8) + 12),
         mem::read<uint32_t>(0x02000000 + (i * 4 * 8) + 16),
         mem::read<uint32_t>(0x02000000 + (i * 4 * 8) + 20)
      };

      espresso::Disassembly disasembly;
      espresso::disassemble(failedInstr, disasembly, failedAddr);

      gLog->warn(" {:08x} = {:08x} failed - {}", failedAddr, failedInstr, disasembly.text);
      gLog->warn("   {:08x} {:08x} {:08x} {:08x}", failedAux[0], failedAux[1], failedAux[2], failedAux[3]);
   }


   return 0;
}

} // namespace actest
