#include <cassert>
#include <experimental/filesystem>
#include <fstream>
#include "cpu/cpu.h"
#include "cpu/jit/jit.h"
#include "cpu/disassembler.h"
#include "hardwaretests.h"
#include "mem/mem.h"
#include "utils/bit_cast.h"
#include "utils/floatutils.h"
#include "utils/log.h"
#include "utils/strutils.h"

namespace fs = std::experimental::filesystem;
static const auto TEST_FPSCR = false;

namespace hwtest
{

static void
printTestField(Field field, Instruction instr, RegisterState *input, RegisterState *output, ThreadState *state)
{
   auto printGPR = [&](uint32_t reg) {
      assert(reg >= GPR_BASE);

      gLog->debug("r{:02d}    =         {:08X}         {:08X}         {:08X}", reg,
                  input->gpr[reg - GPR_BASE],
                  output->gpr[reg - GPR_BASE],
                  state->gpr[reg]);
   };

   auto printFPR = [&](uint32_t reg) {
      assert(reg >= FPR_BASE);

      gLog->debug("f{:02d}    = {:16e} {:16e} {:16e}", reg,
                  input->fr[reg - FPR_BASE],
                  output->fr[reg - FPR_BASE],
                  state->fpr[reg].paired0);

      gLog->debug("         {:16X} {:16X} {:16X}",
                  bit_cast<uint64_t>(input->fr[reg - FPR_BASE]),
                  bit_cast<uint64_t>(output->fr[reg - FPR_BASE]),
                  bit_cast<uint64_t>(state->fpr[reg].paired0));
   };

   switch (field) {
   case Field::rA:
      printGPR(instr.rA);
      break;
   case Field::rB:
      printGPR(instr.rB);
      break;
   case Field::rD:
      printGPR(instr.rS);
      break;
   case Field::rS:
      printGPR(instr.rS);
      break;
   case Field::frA:
      printFPR(instr.frA);
      break;
   case Field::frB:
      printFPR(instr.frB);
      break;
   case Field::frC:
      printFPR(instr.frC);
      break;
   case Field::frD:
      printFPR(instr.frD);
      break;
   case Field::frS:
      printFPR(instr.frS);
      break;
   case Field::XERC:
      gLog->debug("xer.ca =         {:08X}         {:08X}         {:08X}", input->xer.ca, output->xer.ca, state->xer.ca);
      break;
   case Field::XERSO:
      gLog->debug("xer.so =         {:08X}         {:08X}         {:08X}", input->xer.so, output->xer.so, state->xer.so);
      break;
   case Field::FPSCR:
      //gLog->debug("fpscr = {:08X} {:08x} {:08X}", input->fpscr.value, output->fpscr.value, state->fpscr.value);
      break;
   }
}

#define CompareFPSCRField(field) \
   if (result.field != expected.field) { \
      gLog->debug("fpscr." #field " = {} {} {}", input.field, expected.field, result.field); \
      failed = true; \
   }

// Compare all individual fields in fpscr
static bool
compareFPSCR(fpscr_t input, fpscr_t expected, fpscr_t result)
{
   auto failed = false;
   CompareFPSCRField(rn);
   CompareFPSCRField(ni);
   CompareFPSCRField(xe);
   CompareFPSCRField(ze);
   CompareFPSCRField(ue);
   CompareFPSCRField(oe);
   CompareFPSCRField(ve);
   CompareFPSCRField(vxcvi);
   CompareFPSCRField(vxsqrt);
   CompareFPSCRField(vxsoft);
   CompareFPSCRField(fprf);
   CompareFPSCRField(fi);
   CompareFPSCRField(fr);
   CompareFPSCRField(vxvc);
   CompareFPSCRField(vximz);
   CompareFPSCRField(vxzdz);
   CompareFPSCRField(vxidi);
   CompareFPSCRField(vxisi);
   CompareFPSCRField(vxsnan);
   CompareFPSCRField(xx);
   CompareFPSCRField(zx);
   CompareFPSCRField(ux);
   CompareFPSCRField(ox);
   CompareFPSCRField(vx);
   CompareFPSCRField(fex);
   CompareFPSCRField(fx);
   return failed;
}

static const std::vector<std::string>
excludeTests = {
   "fmadds",
   "fmsub", "fmsubs",
   "fnmadd", "fnmadds",
   "fnmsub", "fnmsubs",
};

bool runTests()
{
   uint32_t testsFailed = 0, testsPassed = 0;
   uint32_t baseAddress = 0x02000000;

   // Allocate some memory to write code to
   if (!mem::alloc(baseAddress, 4096)) {
      return false;
   }

   Instruction bclr = gInstructionTable.encode(InstructionID::bclr);
   bclr.bo = 0x1f;
   mem::write(baseAddress + 4, bclr.value);

   // Read all tests
   for (auto &entry : fs::directory_iterator("tests/cpu/wiiu")) {
      std::ifstream file(entry.path().string(), std::ifstream::in | std::ifstream::binary);
      cereal::BinaryInputArchive cerealInput(file);
      TestFile testFile;

      // Parse test file with cereal
      testFile.name = entry.path().filename().string();
      cerealInput(testFile);

      // Skip excluded tests
      if (std::find(excludeTests.begin(), excludeTests.end(), testFile.name) != excludeTests.end()) {
         gLog->info("Skipping {}", testFile.name);
         continue;
      }

      // Run tests
      gLog->info("Checking {}", testFile.name);

      for (auto &test : testFile.tests) {
         ThreadState state;
         bool failed = false;

         // Setup thread state from test input
         memset(&state, 0, sizeof(ThreadState));
         state.cia = 0;
         state.nia = baseAddress;
         state.xer = test.input.xer;
         state.cr = test.input.cr;
         state.fpscr = test.input.fpscr;
         state.ctr = test.input.ctr;

         for (auto i = 0; i < 4; ++i) {
            state.gpr[i + 3] = test.input.gpr[i];
            state.fpr[i + 1].paired0 = test.input.fr[i];
         }

         // Execute test
         mem::write(baseAddress, test.instr.value);
         cpu::jit::clearCache();
         cpu::executeSub(nullptr, &state);

         // Check XER (all bits)
         if (state.xer.value != test.output.xer.value) {
            gLog->error("Test failed, xer expected {:08X} found {:08X}", test.output.xer.value, state.xer.value);
            failed = true;
         }

         // Check Condition Register (all bits)
         if (state.cr.value != test.output.cr.value) {
            gLog->error("Test failed, cr expected {:08X} found {:08X}", test.output.cr.value, state.cr.value);
            failed = true;
         }

         // Check FPSCR (all bits)
         if (TEST_FPSCR) {
            if (state.fpscr.value != test.output.fpscr.value) {
               gLog->error("Test failed, fpscr {:08X} found {:08X}", test.output.fpscr.value, state.fpscr.value);
               failed = true;
            }
         }

         // Check CTR
         if (state.ctr != test.output.ctr) {
            gLog->error("Test failed, ctr expected {:08X} found {:08X}", test.output.ctr, state.ctr);
            failed = true;
         }

         // Check all GPR
         for (auto i = 0; i < 4; ++i) {
            auto reg = i + hwtest::GPR_BASE;
            auto value = state.gpr[reg];
            auto expected = test.output.gpr[i];

            if (value != expected) {
               gLog->error("Test failed, r{} expected {:08X} found {:08X}", reg, expected, value);
               failed = true;
            }
         }

         // Check all FPR
         for (auto i = 0; i < 4; ++i) {
            auto reg = i + hwtest::FPR_BASE;
            auto value = state.fpr[reg].paired0;
            auto expected = test.output.fr[i];

            if (!is_nan(value) && !is_nan(expected) && !is_infinity(value) && !is_infinity(expected)) {
               double dval = value / expected;

               if (dval < 0.999 || dval > 1.001) {
                  gLog->error("Test failed, f{} expected {:16f} found {:16f}", reg, expected, value);
                  failed = true;
               }
            } else {
               if (is_nan(value) && is_nan(expected)) {
                  auto bits = get_float_bits(value);
                  bits.sign = get_float_bits(expected).sign;
                  value = bits.v;
               }

               if (bit_cast<uint64_t>(value) != bit_cast<uint64_t>(expected)) {
                  gLog->error("Test failed, f{} expected {:16X} found {:16X}", reg, bit_cast<uint64_t>(expected), bit_cast<uint64_t>(value));
                  failed = true;
               }
            }
         }

         if (failed) {
            Disassembly dis;

            // Print disassembly
            gDisassembler.disassemble(test.instr, dis, baseAddress);
            gLog->debug(dis.text);

            // Print all test fields
            gLog->debug("{:08x}            Input         Hardware           Interp", test.instr.value);

            for (auto field : dis.instruction->read) {
               printTestField(field, test.instr, &test.input, &test.output, &state);
            }

            for (auto field : dis.instruction->write) {
               printTestField(field, test.instr, &test.input, &test.output, &state);
            }

            for (auto field : dis.instruction->flags) {
               printTestField(field, test.instr, &test.input, &test.output, &state);
            }

            gLog->debug("");
            ++testsFailed;
         } else {
            ++testsPassed;
         }
      }
   }

   gLog->info("Passed: {}, Failed: {}", testsPassed, testsFailed);
   return true;
}

} // namespace hwtest
