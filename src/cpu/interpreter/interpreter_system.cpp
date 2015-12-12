#include <cassert>
#include "cpu/cpu.h"
#include "interpreter_insreg.h"
#include "memory_translate.h"
#include "utils/bitutils.h"
#include "utils/align.h"
#include "utils/log.h"

static SprEncoding
decodeSPR(Instruction instr)
{
   return static_cast<SprEncoding>(((instr.spr << 5) & 0x3E0) | ((instr.spr >> 5) & 0x1F));
}

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
icbi(ThreadState *state, Instruction instr)
{
}

// Data Cache Block Flush
static void
dcbf(ThreadState *state, Instruction instr)
{
}

// Data Cache Block Invalidate
static void
dcbi(ThreadState *state, Instruction instr)
{
}

// Data Cache Block Store
static void
dcbst(ThreadState *state, Instruction instr)
{
}

// Data Cache Block Touch
static void
dcbt(ThreadState *state, Instruction instr)
{
}

// Data Cache Block Touch for Store
static void
dcbtst(ThreadState *state, Instruction instr)
{
}

// Data Cache Block Zero
static void
dcbz(ThreadState *state, Instruction instr)
{
   uint32_t addr;

   if (instr.rA == 0) {
      addr = 0;
   } else {
      addr = state->gpr[instr.rA];
   }

   addr += state->gpr[instr.rB];
   addr = align_down(addr, 32);
   memset(memory_translate(addr), 0, 32);
}

// Data Cache Block Zero Locked
static void
dcbz_l(ThreadState *state, Instruction instr)
{
   dcbz(state, instr);
}

// Enforce In-Order Execution of I/O
static void
eieio(ThreadState *state, Instruction instr)
{
}

// Synchronise
static void
sync(ThreadState *state, Instruction instr)
{
}

// Instruction Synchronise
static void
isync(ThreadState *state, Instruction instr)
{
}

// Move from Special Purpose Register
static void
mfspr(ThreadState *state, Instruction instr)
{
   auto spr = decodeSPR(instr);
   auto value = 0u;

   switch (spr) {
   case SprEncoding::XER:
      value = state->xer.value;
      break;
   case SprEncoding::LR:
      value = state->lr;
      break;
   case SprEncoding::CTR:
      value = state->ctr;
      break;
   case SprEncoding::UGQR0:
      value = state->gqr[0].value;
      break;
   case SprEncoding::UGQR1:
      value = state->gqr[1].value;
      break;
   case SprEncoding::UGQR2:
      value = state->gqr[2].value;
      break;
   case SprEncoding::UGQR3:
      value = state->gqr[3].value;
      break;
   case SprEncoding::UGQR4:
      value = state->gqr[4].value;
      break;
   case SprEncoding::UGQR5:
      value = state->gqr[5].value;
      break;
   case SprEncoding::UGQR6:
      value = state->gqr[6].value;
      break;
   case SprEncoding::UGQR7:
      value = state->gqr[7].value;
      break;
   default:
      gLog->error("Invalid mfspr SPR {}", static_cast<uint32_t>(spr));
   }

   state->gpr[instr.rD] = value;
}

// Move to Special Purpose Register
static void
mtspr(ThreadState *state, Instruction instr)
{
   auto spr = decodeSPR(instr);
   auto value = state->gpr[instr.rS];

   switch (spr) {
   case SprEncoding::XER:
      state->xer.value = value;
      break;
   case SprEncoding::LR:
      state->lr = value;
      break;
   case SprEncoding::CTR:
      state->ctr = value;
      break;
   case SprEncoding::UGQR0:
      state->gqr[0].value = value;
      break;
   case SprEncoding::UGQR1:
      state->gqr[1].value = value;
      break;
   case SprEncoding::UGQR2:
      state->gqr[2].value = value;
      break;
   case SprEncoding::UGQR3:
      state->gqr[3].value = value;
      break;
   case SprEncoding::UGQR4:
      state->gqr[4].value = value;
      break;
   case SprEncoding::UGQR5:
      state->gqr[5].value = value;
      break;
   case SprEncoding::UGQR6:
      state->gqr[6].value = value;
      break;
   case SprEncoding::UGQR7:
      state->gqr[7].value = value;
      break;
   default:
      gLog->error("Invalid mtspr SPR {}", static_cast<uint32_t>(spr));
   }
}

// Move from Time Base Register
static void
mftb(ThreadState *state, Instruction instr)
{
   auto tbr = decodeSPR(instr);
   auto value = 0u;

   switch (tbr) {
   case SprEncoding::UTBL:
      value = state->tbl;
      break;
   case SprEncoding::UTBU:
      value = state->tbu;
      break;
   default:
      gLog->error("Invalid mftb TBR {}", static_cast<uint32_t>(tbr));
   }

   state->gpr[instr.rD] = value;
}

// Move from Machine State Register
static void
mfmsr(ThreadState *state, Instruction instr)
{
   state->gpr[instr.rD] = state->msr.value;
}

// Move to Machine State Register
static void
mtmsr(ThreadState *state, Instruction instr)
{
   state->msr.value = state->gpr[instr.rS];
}

// Move from Segment Register
static void
mfsr(ThreadState *state, Instruction instr)
{
   state->gpr[instr.rD] = state->sr[instr.sr];
}

// Move from Segment Register Indirect
static void
mfsrin(ThreadState *state, Instruction instr)
{
   auto sr = state->gpr[instr.rB] & 0xf;
   state->gpr[instr.rD] = state->sr[sr];
}

// Move to Segment Register
static void
mtsr(ThreadState *state, Instruction instr)
{
   state->sr[instr.sr] = state->gpr[instr.rS];
}

// Move to Segment Register Indirect
static void
mtsrin(ThreadState *state, Instruction instr)
{
   auto sr = state->gpr[instr.rB] & 0xf;
   state->sr[sr] = state->gpr[instr.rS];
}

// Kernel call
static void
kc(ThreadState *state, Instruction instr)
{
   auto id = instr.kcn;

   auto kc = cpu::getKernelCall(id);
   if (!kc) {
      gLog->error("Encountered invalid Kernel Call ID {}", id);
      assert(0);
      return;
   }

   kc->first(state, kc->second);
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
}
