#include <cassert>
#include "bitutils.h"
#include "interpreter.h"
#include "loader.h"
#include "log.h"
#include "system.h"
#include "kernelfunction.h"
#include "usermodule.h"

static SprEncoding
decodeSPR(Instruction instr)
{
   return static_cast<SprEncoding>(instr.sprl | (instr.spru << 5));
}

/*
// System Linkage
INS(rfi, (), (), (), (opcd == 19, xo1 == 50), "")

// Trap
INS(tw, (), (to, ra, rb), (), (opcd == 31, xo1 == 4), "")
INS(twi, (), (to, ra, simm), (), (opcd == 3), "")

// Cache Management
INS(icbi, (), (ra, rb), (), (opcd == 31, xo1 == 982), "")

// Lookaside Buffer Management
INS(tlbie, (), (rb), (), (opcd == 31, xo1 == 306), "")
INS(tlbsync, (), (), (), (opcd == 31, xo1 == 566), "")

// External Control
INS(eciwx, (rd), (ra, rb), (), (opcd == 31, xo1 == 310), "")
INS(ecowx, (rd), (ra, rb), (), (opcd == 31, xo1 == 438), "")
*/

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
   addr = alignDown(addr, 32);
   memset(gMemory.translate(addr), 0, 32);
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
   case SprEncoding::GQR0:
      value = state->gqr[0].value;
      break;
   case SprEncoding::GQR1:
      value = state->gqr[1].value;
      break;
   case SprEncoding::GQR2:
      value = state->gqr[2].value;
      break;
   case SprEncoding::GQR3:
      value = state->gqr[3].value;
      break;
   case SprEncoding::GQR4:
      value = state->gqr[4].value;
      break;
   case SprEncoding::GQR5:
      value = state->gqr[5].value;
      break;
   case SprEncoding::GQR6:
      value = state->gqr[6].value;
      break;
   case SprEncoding::GQR7:
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
   case SprEncoding::GQR0:
      state->gqr[0].value = value;
      break;
   case SprEncoding::GQR1:
      state->gqr[1].value = value;
      break;
   case SprEncoding::GQR2:
      state->gqr[2].value = value;
      break;
   case SprEncoding::GQR3:
      state->gqr[3].value = value;
      break;
   case SprEncoding::GQR4:
      state->gqr[4].value = value;
      break;
   case SprEncoding::GQR5:
      state->gqr[5].value = value;
      break;
   case SprEncoding::GQR6:
      state->gqr[6].value = value;
      break;
   case SprEncoding::GQR7:
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
   case SprEncoding::TBL:
      value = state->tbl;
      break;
   case SprEncoding::TBU:
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
   auto id = instr.li;
   auto implemented = instr.aa;

   if (!implemented) {
      auto userModule = gSystem.getUserModule();
      auto sym = userModule->symbols[id];
      auto fsym = reinterpret_cast<FunctionSymbol*>(sym);

      if (sym->type != SymbolInfo::Function) {
         gLog->error("Attempted to call non-function symbol {}", sym->name);
         return;
      }

      if (!fsym->kernelFunction) {
         gLog->debug("{:08x} unimplemented kernel function {}", state->lr, sym->name);
         return;
      }

      return;
      assert(false);
   }

   auto func = gSystem.getSyscall(id);
   func->call(state);
}

void
Interpreter::registerSystemInstructions()
{
   RegisterInstruction(dcbf);
   RegisterInstruction(dcbi);
   RegisterInstruction(dcbst);
   RegisterInstruction(dcbt);
   RegisterInstruction(dcbtst);
   RegisterInstruction(dcbz);
   RegisterInstruction(dcbz_l);
   RegisterInstruction(eieio);
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
