#include "cpu.h"
#include "cpu_breakpoints.h"
#include "cpu_internal.h"
#include "espresso/espresso_spr.h"
#include "espresso/espresso_instructionset.h"
#include "interpreter_insreg.h"
#include "mem.h"

#include <common/align.h>
#include <common/bitutils.h>
#include <common/decaf_assert.h>
#include <common/log.h>
#include <fmt/format.h>

using espresso::SPR;

/*
// System Linkage
INS(rfi, (), (), (), (opcd == 19, xo1 == 50), "")

// Trap
INS(tw, (), (to, ra, rb), (), (opcd == 31, xo1 == 4), "")
INS(twi, (), (to, ra, simm), (), (opcd == 3), "")

// Lookaside Buffer Management
INS(tlbie, (), (rb), (), (opcd == 31, xo1 == 306), "")
INS(tlbsync, (), (), (), (opcd == 31, xo1 == 566), "")

// External Control
INS(eciwx, (rd), (ra, rb), (), (opcd == 31, xo1 == 310), "")
INS(ecowx, (rd), (ra, rb), (), (opcd == 31, xo1 == 438), "")
*/

// Cache Management
static void
icbi(cpu::Core *state, Instruction instr)
{
}

// Data Cache Block Flush
static void
dcbf(cpu::Core *state, Instruction instr)
{
}

// Data Cache Block Invalidate
static void
dcbi(cpu::Core *state, Instruction instr)
{
}

// Data Cache Block Store
static void
dcbst(cpu::Core *state, Instruction instr)
{
}

// Data Cache Block Touch
static void
dcbt(cpu::Core *state, Instruction instr)
{
}

// Data Cache Block Touch for Store
static void
dcbtst(cpu::Core *state, Instruction instr)
{
}

// Data Cache Block Zero
static void
dcbz(cpu::Core *state, Instruction instr)
{
   uint32_t addr;

   if (instr.rA == 0) {
      addr = 0;
   } else {
      addr = state->gpr[instr.rA];
   }

   addr += state->gpr[instr.rB];
   addr = align_down(addr, 32);
   memset(mem::translate(addr), 0, 32);
}

// Data Cache Block Zero Locked
static void
dcbz_l(cpu::Core *state, Instruction instr)
{
   dcbz(state, instr);
}

// Enforce In-Order Execution of I/O
static void
eieio(cpu::Core *state, Instruction instr)
{
}

// Synchronise
static void
sync(cpu::Core *state, Instruction instr)
{
}

// Instruction Synchronise
static void
isync(cpu::Core *state, Instruction instr)
{
}

// Move from Special Purpose Register
static void
mfspr(cpu::Core *state, Instruction instr)
{
   auto spr = decodeSPR(instr);
   auto value = 0u;

   switch (spr) {
   case SPR::XER:
      value = state->xer.value;
      break;
   case SPR::LR:
      value = state->lr;
      break;
   case SPR::CTR:
      value = state->ctr;
      break;
   case SPR::UGQR0:
      value = state->gqr[0].value;
      break;
   case SPR::UGQR1:
      value = state->gqr[1].value;
      break;
   case SPR::UGQR2:
      value = state->gqr[2].value;
      break;
   case SPR::UGQR3:
      value = state->gqr[3].value;
      break;
   case SPR::UGQR4:
      value = state->gqr[4].value;
      break;
   case SPR::UGQR5:
      value = state->gqr[5].value;
      break;
   case SPR::UGQR6:
      value = state->gqr[6].value;
      break;
   case SPR::UGQR7:
      value = state->gqr[7].value;
      break;
   case SPR::UPIR:
      value = cpu::this_core::id();
      break;
   default:
      gLog->error("Invalid mfspr SPR {}", static_cast<uint32_t>(spr));
   }

   state->gpr[instr.rD] = value;
}

// Move to Special Purpose Register
static void
mtspr(cpu::Core *state, Instruction instr)
{
   auto spr = decodeSPR(instr);
   auto value = state->gpr[instr.rS];

   switch (spr) {
   case SPR::XER:
      state->xer.value = value;
      break;
   case SPR::LR:
      state->lr = value;
      break;
   case SPR::CTR:
      state->ctr = value;
      break;
   case SPR::UGQR0:
      state->gqr[0].value = value;
      break;
   case SPR::UGQR1:
      state->gqr[1].value = value;
      break;
   case SPR::UGQR2:
      state->gqr[2].value = value;
      break;
   case SPR::UGQR3:
      state->gqr[3].value = value;
      break;
   case SPR::UGQR4:
      state->gqr[4].value = value;
      break;
   case SPR::UGQR5:
      state->gqr[5].value = value;
      break;
   case SPR::UGQR6:
      state->gqr[6].value = value;
      break;
   case SPR::UGQR7:
      state->gqr[7].value = value;
      break;
   default:
      decaf_abort(fmt::format("Invalid mtspr SPR {}", static_cast<uint32_t>(spr)));
   }
}

// Move from Time Base Register
static void
mftb(cpu::Core *state, Instruction instr)
{
   auto tbr = decodeSPR(instr);
   auto value = 0u;

   switch (tbr) {
   case SPR::UTBL:
      value = static_cast<uint32_t>(state->tb() & 0xFFFFFFFF);
      break;
   case SPR::UTBU:
      value = static_cast<uint32_t>(state->tb()  >> 32);
      break;
   default:
      decaf_abort(fmt::format("Invalid mftb TBR {}", static_cast<uint32_t>(tbr)));
   }

   state->gpr[instr.rD] = value;
}

// Move from Machine State Register
static void
mfmsr(cpu::Core *state, Instruction instr)
{
   state->gpr[instr.rD] = state->msr.value;
}

// Move to Machine State Register
static void
mtmsr(cpu::Core *state, Instruction instr)
{
   state->msr.value = state->gpr[instr.rS];
}

// Move from Segment Register
static void
mfsr(cpu::Core *state, Instruction instr)
{
   state->gpr[instr.rD] = state->sr[instr.sr];
}

// Move from Segment Register Indirect
static void
mfsrin(cpu::Core *state, Instruction instr)
{
   auto sr = state->gpr[instr.rB] & 0xf;
   state->gpr[instr.rD] = state->sr[sr];
}

// Move to Segment Register
static void
mtsr(cpu::Core *state, Instruction instr)
{
   state->sr[instr.sr] = state->gpr[instr.rS];
}

// Move to Segment Register Indirect
static void
mtsrin(cpu::Core *state, Instruction instr)
{
   auto sr = state->gpr[instr.rB] & 0xf;
   state->sr[sr] = state->gpr[instr.rS];
}

// Kernel call
static void
kc(cpu::Core *state, Instruction instr)
{
   auto kcId = instr.kcn;

   auto handler = cpu::getSystemCallHandler(kcId);
   state->systemCallStackHead = state->gpr[1];
   state = handler(state, kcId);
}

// Trap Word
static void
tw(cpu::Core *state, Instruction instr)
{
   if (!cpu::hasBreakpoint(state->cia)) {
      decaf_abort(fmt::format("Game raised a trap exception at 0x{:08X}.", state->nia));
   }

   // By this point we have already handled the breakpoint thanks to checkInterrupts,
   // so here we just need to execute the breakpoint's saved instruction!
   auto savedInstr = cpu::getBreakpointSavedCode(state->cia);
   auto data = espresso::decodeInstruction(savedInstr);
   decaf_check(data);

   auto handler = cpu::interpreter::getInstructionHandler(data->id);
   decaf_check(handler);

   handler(state, savedInstr);
}

void
cpu::interpreter::registerSystemInstructions()
{
   RegisterInstruction(dcbf);
   RegisterInstruction(dcbi);
   RegisterInstruction(dcbst);
   RegisterInstruction(dcbt);
   RegisterInstruction(dcbtst);
   RegisterInstruction(dcbz);
   RegisterInstruction(dcbz_l);
   RegisterInstruction(eieio);
   RegisterInstruction(icbi);
   RegisterInstruction(isync);
   RegisterInstruction(sync);
   RegisterInstruction(mfspr);
   RegisterInstruction(mtspr);
   RegisterInstruction(mftb);
   RegisterInstruction(mfmsr);
   RegisterInstruction(mtmsr);
   RegisterInstruction(mfsr);
   RegisterInstruction(mfsrin);
   RegisterInstruction(mtsr);
   RegisterInstruction(mtsrin);
   RegisterInstruction(kc);
   RegisterInstruction(tw);
}
