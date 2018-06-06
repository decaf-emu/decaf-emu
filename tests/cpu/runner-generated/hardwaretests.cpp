#include "hardwaretests.h"

#include <cassert>
#include <cfenv>
#include <common/bit_cast.h>
#include <common/floatutils.h>
#include <common/log.h>
#include <common/strutils.h>
#include <fstream>
#include <libcpu/cpu.h>
#include <libcpu/mem.h>
#include <libcpu/espresso/espresso_disassembler.h>
#include <libcpu/espresso/espresso_instructionset.h>
#include <libcpu/espresso/espresso_registerformats.h>
#include <libdecaf/src/filesystem/filesystem.h>

using namespace espresso;

static const auto TEST_FPSCR = false;
static const auto TEST_FPSCR_FR = false;
static const auto TEST_FPSCR_UX = false;
static const auto TEST_FMADDSUB = false;

namespace hwtest
{

static void
printTestField(InstructionField field, Instruction instr, RegisterState *input, RegisterState *output, cpu::CoreRegs *state)
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
                  state->fpr[reg].value);

      gLog->debug("         {:16X} {:16X} {:16X}",
                  bit_cast<uint64_t>(input->fr[reg - FPR_BASE]),
                  bit_cast<uint64_t>(output->fr[reg - FPR_BASE]),
                  bit_cast<uint64_t>(state->fpr[reg].value));
   };

   switch (field) {
   case InstructionField::rA:
      printGPR(instr.rA);
      break;
   case InstructionField::rB:
      printGPR(instr.rB);
      break;
   case InstructionField::rD:
      printGPR(instr.rS);
      break;
   case InstructionField::rS:
      printGPR(instr.rS);
      break;
   case InstructionField::frA:
      printFPR(instr.frA);
      break;
   case InstructionField::frB:
      printFPR(instr.frB);
      break;
   case InstructionField::frC:
      printFPR(instr.frC);
      break;
   case InstructionField::frD:
      printFPR(instr.frD);
      break;
   case InstructionField::frS:
      printFPR(instr.frS);
      break;
   case InstructionField::XERC:
      gLog->debug("xer.ca =         {:08X}         {:08X}         {:08X}", input->xer.ca, output->xer.ca, state->xer.ca);
      break;
   case InstructionField::XERSO:
      gLog->debug("xer.so =         {:08X}         {:08X}         {:08X}", input->xer.so, output->xer.so, state->xer.so);
      break;
   case InstructionField::FPSCR:
      gLog->debug("fpscr =          {:08X}         {:08x}         {:08X}", input->fpscr.value, output->fpscr.value, state->fpscr.value);
      break;
   default:
      break;
   }
}

#define CompareFPSCRField(field) \
   if (result.field != expected.field) { \
      gLog->debug("fpscr." #field " = input {} expected {} found {}", input.field, expected.field, result.field); \
      failed = true; \
   }

// Compare all individual fields in fpscr
static bool
compareFPSCR(FloatingPointStatusAndControlRegister input,
             FloatingPointStatusAndControlRegister expected,
             FloatingPointStatusAndControlRegister result)
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

int runTests(const std::string &path)
{
   uint32_t testsFailed = 0, testsPassed = 0;
   auto baseAddress = cpu::VirtualAddress { 0x02000000u };
   auto basePhysicalAddress = cpu::PhysicalAddress { 0x50000000u };
   auto codeSize = 2048u;

   cpu::allocateVirtualAddress(baseAddress, codeSize);
   cpu::mapMemory(baseAddress, basePhysicalAddress, codeSize, cpu::MapPermission::ReadWrite);

   Instruction bclr = encodeInstruction(InstructionID::bclr);
   bclr.bo = 0x1f;
   mem::write(baseAddress.getAddress() + 4, bclr.value);

   fs::FileSystem filesystem;
   fs::FolderEntry entry;
   fs::HostPath base = path;
   filesystem.mountHostFolder("/tests", base, fs::Permissions::Read);
   auto fsResult = filesystem.openFolder("/tests");

   if (!fsResult) {
      return -1;
   }

   auto folder = fsResult.value();

   while (folder->read(entry)) {
      std::ifstream file(base.join(entry.name).path(), std::ifstream::in | std::ifstream::binary);
      cereal::BinaryInputArchive cerealInput(file);
      TestFile testFile;

      // Parse test file with cereal
      testFile.name = entry.name;
      cerealInput(testFile);

      // Run tests
      for (auto &test : testFile.tests) {
         bool failed = false;

         if (!TEST_FMADDSUB) {
            auto data = espresso::decodeInstruction(test.instr);
            switch (data->id) {
            case InstructionID::fmadd:
            case InstructionID::fmadds:
            case InstructionID::fmsub:
            case InstructionID::fmsubs:
            case InstructionID::fnmadd:
            case InstructionID::fnmadds:
            case InstructionID::fnmsub:
            case InstructionID::fnmsubs:
               failed = true;
               break;
            }
            if (failed) {
               continue;
            }
         }

         // Setup core state from test input
         cpu::CoreRegs *state = cpu::this_core::state();
         memset(state, 0, sizeof(cpu::CoreRegs));
         state->cia = 0;
         state->nia = baseAddress.getAddress();
         state->xer = test.input.xer;
         state->cr = test.input.cr;
         state->fpscr = test.input.fpscr;
         state->ctr = test.input.ctr;

         for (auto i = 0; i < 4; ++i) {
            state->gpr[i + 3] = test.input.gpr[i];
            state->fpr[i + 1].paired0 = test.input.fr[i];
         }

         // Execute test
         mem::write(baseAddress.getAddress(), test.instr.value);
         cpu::clearInstructionCache();
         cpu::this_core::executeSub();

         // Check XER (all bits)
         if (state->xer.value != test.output.xer.value) {
            gLog->error("Test failed, xer expected {:08X} found {:08X}", test.output.xer.value, state->xer.value);
            failed = true;
         }

         // Check Condition Register (all bits)
         if (state->cr.value != test.output.cr.value) {
            gLog->error("Test failed, cr expected {:08X} found {:08X}", test.output.cr.value, state->cr.value);
            failed = true;
         }

         // Check FPSCR
         if (TEST_FPSCR) {
            if (!TEST_FPSCR_FR) {
               state->fpscr.fr = 0;
               test.output.fpscr.fr = 0;
            }

            if (!TEST_FPSCR_UX) {
               state->fpscr.ux = 0;
               test.output.fpscr.ux = 0;
            }

            auto state_fpscr = state->fpscr.value;
            auto test_fpscr = test.output.fpscr.value;

            if (state_fpscr != test_fpscr) {
               gLog->error("Test failed, fpscr {:08X} found {:08X}", test.output.fpscr.value, state->fpscr.value);
               compareFPSCR(test.input.fpscr, state->fpscr, test.output.fpscr);
               failed = true;
            }
         }

         // Check CTR
         if (state->ctr != test.output.ctr) {
            gLog->error("Test failed, ctr expected {:08X} found {:08X}", test.output.ctr, state->ctr);
            failed = true;
         }

         // Check all GPR
         for (auto i = 0; i < 4; ++i) {
            auto reg = i + hwtest::GPR_BASE;
            auto value = state->gpr[reg];
            auto expected = test.output.gpr[i];

            if (value != expected) {
               gLog->error("Test failed, r{} expected {:08X} found {:08X}", reg, expected, value);
               failed = true;
            }
         }

         // Check all FPR
         for (auto i = 0; i < 4; ++i) {
            auto reg = i + hwtest::FPR_BASE;
            auto value = state->fpr[reg].value;
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
            disassemble(test.instr, dis, baseAddress.getAddress());
            gLog->debug(dis.text);

            // Print all test fields
            gLog->debug("{:08x}            Input         Hardware           Interp", test.instr.value);

            for (auto field : dis.instruction->read) {
               printTestField(field, test.instr, &test.input, &test.output, state);
            }

            for (auto field : dis.instruction->write) {
               printTestField(field, test.instr, &test.input, &test.output, state);
            }

            for (auto field : dis.instruction->flags) {
               printTestField(field, test.instr, &test.input, &test.output, state);
            }

            gLog->debug("");
            ++testsFailed;
         } else {
            ++testsPassed;
         }
      }
   }

   if (testsFailed) {
      gLog->error("Failed {} of {} tests.", testsFailed, testsFailed + testsPassed);
   }

   return testsFailed;
}

} // namespace hwtest
