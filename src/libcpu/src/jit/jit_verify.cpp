#include "common/align.h"
#include "common/bitutils.h"
#include "common/byte_swap.h"
#include "common/decaf_assert.h"
#include "common/log.h"
#include "espresso/espresso_disassembler.h"
#include "espresso/espresso_instructionset.h"
#include "interpreter/interpreter_insreg.h"
#include "jit_internal.h"
#include "jit_verify.h"
#include "mem.h"


namespace cpu
{

namespace jit
{

// Ensure load/store verification is not broken by other threads
static std::mutex
memoryLock;

void
insertVerifyCall(PPCEmuAssembler &a,
                 uint32_t instr,
                 void *verifyWrapper)
{
   a.saveAll();
   a.mov(asmjit::X86Mem(asmjit::x86::rsp, 36, 4), instr);
   a.call(asmjit::Ptr(verifyWrapper));
}

static bool
isMemoryInstruction(uint32_t instr)
{
   auto data = espresso::decodeInstruction(instr);
   return data->id == espresso::InstructionID::lbz
       || data->id == espresso::InstructionID::lbzu
       || data->id == espresso::InstructionID::lbzx
       || data->id == espresso::InstructionID::lbzux
       || data->id == espresso::InstructionID::lhz
       || data->id == espresso::InstructionID::lhzu
       || data->id == espresso::InstructionID::lhzx
       || data->id == espresso::InstructionID::lhzux
       || data->id == espresso::InstructionID::lhbrx
       || data->id == espresso::InstructionID::lha
       || data->id == espresso::InstructionID::lhau
       || data->id == espresso::InstructionID::lhax
       || data->id == espresso::InstructionID::lhaux
       || data->id == espresso::InstructionID::lwz
       || data->id == espresso::InstructionID::lwzu
       || data->id == espresso::InstructionID::lwzx
       || data->id == espresso::InstructionID::lwzux
       || data->id == espresso::InstructionID::lwbrx
       || data->id == espresso::InstructionID::lwarx
       || data->id == espresso::InstructionID::lfs
       || data->id == espresso::InstructionID::lfsu
       || data->id == espresso::InstructionID::lfsx
       || data->id == espresso::InstructionID::lfsux
       || data->id == espresso::InstructionID::lfd
       || data->id == espresso::InstructionID::lfdu
       || data->id == espresso::InstructionID::lfdx
       || data->id == espresso::InstructionID::lfdux
       || data->id == espresso::InstructionID::lmw
       || data->id == espresso::InstructionID::lswi
       || data->id == espresso::InstructionID::lswx
       || data->id == espresso::InstructionID::psq_l
       || data->id == espresso::InstructionID::psq_lu
       || data->id == espresso::InstructionID::psq_lx
       || data->id == espresso::InstructionID::psq_lux
       || data->id == espresso::InstructionID::stb
       || data->id == espresso::InstructionID::stbu
       || data->id == espresso::InstructionID::stbx
       || data->id == espresso::InstructionID::stbux
       || data->id == espresso::InstructionID::sth
       || data->id == espresso::InstructionID::sthu
       || data->id == espresso::InstructionID::sthx
       || data->id == espresso::InstructionID::sthux
       || data->id == espresso::InstructionID::sthbrx
       || data->id == espresso::InstructionID::stw
       || data->id == espresso::InstructionID::stwu
       || data->id == espresso::InstructionID::stwx
       || data->id == espresso::InstructionID::stwux
       || data->id == espresso::InstructionID::stwbrx
       || data->id == espresso::InstructionID::stwcx
       || data->id == espresso::InstructionID::stfs
       || data->id == espresso::InstructionID::stfsu
       || data->id == espresso::InstructionID::stfsx
       || data->id == espresso::InstructionID::stfsux
       || data->id == espresso::InstructionID::stfiwx
       || data->id == espresso::InstructionID::stfd
       || data->id == espresso::InstructionID::stfdu
       || data->id == espresso::InstructionID::stfdx
       || data->id == espresso::InstructionID::stfdux
       || data->id == espresso::InstructionID::stmw
       || data->id == espresso::InstructionID::stswi
       || data->id == espresso::InstructionID::stswx
       || data->id == espresso::InstructionID::dcbz
       || data->id == espresso::InstructionID::dcbz_l
       || data->id == espresso::InstructionID::psq_st
       || data->id == espresso::InstructionID::psq_stu
       || data->id == espresso::InstructionID::psq_stx
       || data->id == espresso::InstructionID::psq_stux
;
}

static void
lookupMemoryTarget(VerifyBuffer *verifyBuf,
                   espresso::Instruction instr)
{
   auto core = &verifyBuf->coreCopy;
   auto data = espresso::decodeInstruction(instr);

   if (!data) {  // Instruction word was invalid
      verifyBuf->memorySize = 0;
      return;
   }

   // Calculate size and address separately to reduce code duplication

   switch (data->id) {
   case espresso::InstructionID::stb:
   case espresso::InstructionID::stbu:
   case espresso::InstructionID::stbx:
   case espresso::InstructionID::stbux:
      verifyBuf->memorySize = 1;
      break;
   case espresso::InstructionID::sth:
   case espresso::InstructionID::sthu:
   case espresso::InstructionID::sthx:
   case espresso::InstructionID::sthux:
   case espresso::InstructionID::sthbrx:
      verifyBuf->memorySize = 2;
      break;

   case espresso::InstructionID::stw:
   case espresso::InstructionID::stwu:
   case espresso::InstructionID::stwx:
   case espresso::InstructionID::stwux:
   case espresso::InstructionID::stwbrx:
   case espresso::InstructionID::stwcx:
   case espresso::InstructionID::stfs:
   case espresso::InstructionID::stfsu:
   case espresso::InstructionID::stfsx:
   case espresso::InstructionID::stfsux:
   case espresso::InstructionID::stfiwx:
      verifyBuf->memorySize = 4;
      break;

   case espresso::InstructionID::stfd:
   case espresso::InstructionID::stfdu:
   case espresso::InstructionID::stfdx:
   case espresso::InstructionID::stfdux:
      verifyBuf->memorySize = 8;
      break;

   case espresso::InstructionID::stmw:
      verifyBuf->memorySize = 4 * (32 - instr.rS);
      break;

   case espresso::InstructionID::stswi:
      verifyBuf->memorySize = instr.nb;
      break;

   case espresso::InstructionID::stswx:
      verifyBuf->memorySize = core->xer.byteCount;
      break;

   case espresso::InstructionID::dcbz:
   case espresso::InstructionID::dcbz_l:
      verifyBuf->memorySize = 32;
      break;

   case espresso::InstructionID::psq_st:
   case espresso::InstructionID::psq_stu:
   case espresso::InstructionID::psq_stx:
   case espresso::InstructionID::psq_stux:
      // TODO (fall through to default for now)

   default:
      verifyBuf->memorySize = 0;
      return;
   }

   switch (data->id) {
   case espresso::InstructionID::stb:
   case espresso::InstructionID::stbu:
   case espresso::InstructionID::sth:
   case espresso::InstructionID::sthu:
   case espresso::InstructionID::stw:
   case espresso::InstructionID::stwu:
   case espresso::InstructionID::stmw:
   case espresso::InstructionID::stfs:
   case espresso::InstructionID::stfsu:
   case espresso::InstructionID::stfd:
   case espresso::InstructionID::stfdu:
      if (instr.rA == 0) {
         verifyBuf->memoryAddress = 0;
      } else {
         verifyBuf->memoryAddress = core->gpr[instr.rA];
      }
      verifyBuf->memoryAddress += sign_extend<16, int32_t>(instr.d);
      break;

   case espresso::InstructionID::stbx:
   case espresso::InstructionID::stbux:
   case espresso::InstructionID::sthx:
   case espresso::InstructionID::sthux:
   case espresso::InstructionID::sthbrx:
   case espresso::InstructionID::stwx:
   case espresso::InstructionID::stwux:
   case espresso::InstructionID::stwbrx:
   case espresso::InstructionID::stwcx:
   case espresso::InstructionID::stswx:
   case espresso::InstructionID::stfsx:
   case espresso::InstructionID::stfsux:
   case espresso::InstructionID::stfiwx:
   case espresso::InstructionID::stfdx:
   case espresso::InstructionID::stfdux:
      if (instr.rA == 0) {
         verifyBuf->memoryAddress = 0;
      } else {
         verifyBuf->memoryAddress = core->gpr[instr.rA];
      }
      verifyBuf->memoryAddress += core->gpr[instr.rB];
      break;

   case espresso::InstructionID::stswi:
      if (instr.rA == 0) {
         verifyBuf->memoryAddress = 0;
      } else {
         verifyBuf->memoryAddress = core->gpr[instr.rA];
      }
      break;

   case espresso::InstructionID::dcbz:
   case espresso::InstructionID::dcbz_l:
      if (instr.rA == 0) {
         verifyBuf->memoryAddress = 0;
      } else {
         verifyBuf->memoryAddress = core->gpr[instr.rA];
      }
      verifyBuf->memoryAddress += core->gpr[instr.rB];
      verifyBuf->memoryAddress = align_down(verifyBuf->memoryAddress, 32);
      break;

   case espresso::InstructionID::psq_st:
   case espresso::InstructionID::psq_stu:
   case espresso::InstructionID::psq_stx:
   case espresso::InstructionID::psq_stux:
      // TODO (fall through to default for now)

   default:
      decaf_abort("Missing memoryAddress calculation");
   }
}

// Helper for verifyPost() so we don't have to pay the disassembly cost
//  if the instruction worked as expected.
static std::string
disassemble(uint32_t instr,
            uint32_t address)
{
   espresso::Disassembly disassembly;
   espresso::disassemble(static_cast<espresso::Instruction>(instr), disassembly, address);
   return disassembly.text;
}

void
verifyPre(Core *core,
          VerifyBuffer *verifyBuf,
          uint32_t instr)
{
   // If entering a load/store instruction, lock out other cores so we can
   //  safely verify the instruction's behavior
   if (isMemoryInstruction(instr)) {
      memoryLock.lock();
   }

   // Save the current core state for interpreter execution in verifyPost()
   memcpy(&verifyBuf->coreCopy, core, sizeof(*core));

   // Save the initial contents of any memory touched by the instruction
   lookupMemoryTarget(verifyBuf, static_cast<espresso::Instruction>(instr));
   decaf_check(verifyBuf->memorySize <= sizeof(verifyBuf->preJitBuffer));
   memcpy(verifyBuf->preJitBuffer, mem::translate(verifyBuf->memoryAddress), verifyBuf->memorySize);
}

void
verifyPost(Core *core,
           VerifyBuffer *verifyBuf,
           uint32_t instr)
{
   auto coreCopy = &verifyBuf->coreCopy;
   auto cia = coreCopy->cia;

   // Save the data written by JIT code...
   memcpy(verifyBuf->postJitBuffer, mem::translate(verifyBuf->memoryAddress), verifyBuf->memorySize);
   // ... and restore the original data for the interpreter
   memcpy(mem::translate(verifyBuf->memoryAddress), verifyBuf->preJitBuffer, verifyBuf->memorySize);

   // Execute the instruction using the interpreter implementation
   auto data = espresso::decodeInstruction(instr);
   if (data) {
      auto fptr = interpreter::getInstructionHandler(data->id);
      decaf_assert(fptr, fmt::format("Unimplemented interpreter instruction {}", data->name));
      coreCopy->nia = cia + 4;
      fptr(coreCopy, instr);
   }

   // Check all registers and any touched memory for discrepancies

   decaf_assert(core->nia == coreCopy->nia,
                fmt::format("Wrong NIA at 0x{:X}: {}\n      Found: 0x{:08X}\n   Expected: 0x{:08X}",
                            cia, disassemble(instr, cia),
                            core->nia, coreCopy->nia));

   for (auto i = 0; i < 32; ++i) {
      decaf_assert(core->gpr[i] == coreCopy->gpr[i],
                   fmt::format("Wrong value in GPR {} at 0x{:X}: {}\n      Found: 0x{:08X}\n   Expected: 0x{:08X}",
                               i, cia, disassemble(instr, cia),
                               core->gpr[i], coreCopy->gpr[i]));
   }

   for (auto i = 0; i < 32; ++i) {
      decaf_assert(core->fpr[i].idw == coreCopy->fpr[i].idw,
                   fmt::format("Wrong value in FPR {} at 0x{:X}: {}\n      Found: 0x{:08X}_{:08X} ({:g})\n   Expected: 0x{:08X}_{:08X} ({:g})",
                               i, cia, disassemble(instr, cia),
                               static_cast<uint32_t>(core->fpr[i].idw >> 32),
                               static_cast<uint32_t>(core->fpr[i].idw),
                               core->fpr[i].value,
                               static_cast<uint32_t>(coreCopy->fpr[i].idw >> 32),
                               static_cast<uint32_t>(coreCopy->fpr[i].idw),
                               coreCopy->fpr[i].value));
   }

   for (auto i = 0; i < 32; ++i) {
      decaf_assert(core->fpr[i].idw_paired1 == coreCopy->fpr[i].idw_paired1,
                   fmt::format("Wrong value in PS1 {} at 0x{:X}: {}\n      Found: 0x{:08X}_{:08X} ({:g})\n   Expected: 0x{:08X}_{:08X} ({:g})",
                               i, cia, disassemble(instr, cia),
                               static_cast<uint32_t>(core->fpr[i].idw_paired1 >> 32),
                               static_cast<uint32_t>(core->fpr[i].idw_paired1),
                               core->fpr[i].paired1,
                               static_cast<uint32_t>(coreCopy->fpr[i].idw_paired1 >> 32),
                               static_cast<uint32_t>(coreCopy->fpr[i].idw_paired1),
                               coreCopy->fpr[i].paired1));
   }

   for (auto i = 0; i < 8; ++i) {
      decaf_assert(core->gqr[i].value == coreCopy->gqr[i].value,
                   fmt::format("Wrong value in GQR {} at 0x{:X}: {}\n      Found: 0x{:08X}\n   Expected: 0x{:08X}",
                               i, cia, disassemble(instr, cia),
                               core->gqr[i].value, coreCopy->gqr[i].value));
   }

   decaf_assert(core->lr == coreCopy->lr,
                fmt::format("Wrong value in LR at 0x{:X}: {}\n      Found: 0x{:08X}\n   Expected: 0x{:08X}",
                            cia, disassemble(instr, cia),
                            core->lr, coreCopy->lr));

   decaf_assert(core->ctr == coreCopy->ctr,
                fmt::format("Wrong value in CTR at 0x{:X}: {}\n      Found: 0x{:08X}\n   Expected: 0x{:08X}",
                            cia, disassemble(instr, cia),
                            core->ctr, coreCopy->ctr));

   decaf_assert(core->cr.value == coreCopy->cr.value,
                fmt::format("Wrong value in CR at 0x{:X}: {}\n      Found: 0x{:08X}\n   Expected: 0x{:08X}",
                            cia, disassemble(instr, cia),
                            core->cr.value, coreCopy->cr.value));

   decaf_assert(core->xer.value == coreCopy->xer.value,
                fmt::format("Wrong value in XER at 0x{:X}: {}\n      Found: 0x{:08X}\n   Expected: 0x{:08X}",
                            cia, disassemble(instr, cia),
                            core->xer.value, coreCopy->xer.value));

   if (0)  // TODO: re-enable when JIT FP implementations update FPSCR
   decaf_assert(core->fpscr.value == coreCopy->fpscr.value,
                fmt::format("Wrong value in FPSCR at 0x{:X}: {}\n      Found: 0x{:08X}\n   Expected: 0x{:08X}",
                            cia, disassemble(instr, cia),
                            core->fpscr.value, coreCopy->fpscr.value));

   for (auto i = 0; i < verifyBuf->memorySize; ++i) {
      auto found = verifyBuf->postJitBuffer[i];
      auto expected = mem::read<uint8_t>(verifyBuf->memoryAddress + i);
      if (found != expected) {
         // Try and make the output reasonably useful
         std::string addressStr = fmt::format("0x{:X}", verifyBuf->memoryAddress);
         std::string foundStr, expectedStr;
         if (data->id == espresso::InstructionID::stswi || data->id == espresso::InstructionID::stswx) {
            addressStr += fmt::format("+0x{:X}", i);
            foundStr = fmt::format("0x{:02X}", found);
            expectedStr = fmt::format("0x{:02X}", expected);
         } else if (data->id == espresso::InstructionID::stmw) {
            auto offset = align_down(i, 4);
            addressStr += fmt::format("+0x{:X}", offset);
            foundStr = fmt::format("0x{:02X}", byte_swap(*reinterpret_cast<uint32_t *>(&verifyBuf->postJitBuffer[offset])));
            expectedStr = fmt::format("0x{:02X}", mem::read<uint32_t>(verifyBuf->memoryAddress + offset));
         } else if (verifyBuf->memorySize == 8) {
            foundStr = fmt::format("0x{:08X}_{:08X}",
                                   byte_swap(*reinterpret_cast<uint32_t *>(verifyBuf->postJitBuffer)),
                                   byte_swap(*reinterpret_cast<uint32_t *>(&verifyBuf->postJitBuffer[4])));
            expectedStr = fmt::format("0x{:08X}_{:08X}",
                                      mem::read<uint32_t>(verifyBuf->memoryAddress),
                                      mem::read<uint32_t>(verifyBuf->memoryAddress + 4));
         } else if (verifyBuf->memorySize == 4) {
            foundStr = fmt::format("0x{:08X}", byte_swap(*reinterpret_cast<uint32_t *>(verifyBuf->postJitBuffer)));
            expectedStr = fmt::format("0x{:08X}", mem::read<uint32_t>(verifyBuf->memoryAddress));
         } else if (verifyBuf->memorySize == 2) {
            foundStr = fmt::format("0x{:04X}", byte_swap(*reinterpret_cast<uint16_t *>(verifyBuf->postJitBuffer)));
            expectedStr = fmt::format("0x{:04X}", mem::read<uint16_t>(verifyBuf->memoryAddress));
         } else {
            foundStr = fmt::format("0x{:02X}", found);
            expectedStr = fmt::format("0x{:02X}", expected);
         }
         decaf_abort(fmt::format("Wrong data written to {} at 0x{:X}: {}\n      Found: {}\n   Expected: {}",
                                 addressStr, cia, disassemble(instr, cia),
                                 foundStr, expectedStr));
      }
   }

   // If this was a load/store instruction, let other cores proceed again
   if (isMemoryInstruction(instr)) {
      memoryLock.unlock();
   }
}

} // namespace jit

} // namespace cpu
