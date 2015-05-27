#include <cassert>
#include "bitutils.h"
#include "interpreter.h"
#include "loader.h"
#include "log.h"
#include "system.h"

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
INS(dcbf, (), (ra, rb), (), (opcd == 31, xo1 == 86), "")
INS(dcbi, (), (ra, rb), (), (opcd == 31, xo1 == 470), "")
INS(dcbst, (), (ra, rb), (), (opcd == 31, xo1 == 54), "")
INS(dcbt, (), (ra, rb), (), (opcd == 31, xo1 == 278), "")
INS(dcbtst, (), (ra, rb), (), (opcd == 31, xo1 == 246), "")
INS(dcbz, (), (ra, rb), (), (opcd == 31, xo1 == 1014), "")
INS(lcbi, (), (ra, rb), (), (opcd == 31, xo1 == 982), "")

// Memory Synchronisation
INS(eieio, (), (), (), (opcd == 31, xo1 == 854), "")
INS(isync, (), (), (), (opcd == 19, xo1 == 150), "")
INS(sync, (), (), (), (opcd == 31, xo1 == 598), "")

// Lookaside Buffer Management
INS(tlbie, (), (rb), (), (opcd == 31, xo1 == 306), "")
INS(tlbsync, (), (), (), (opcd == 31, xo1 == 566), "")

// External Control
INS(eciwx, (rd), (ra, rb), (), (opcd == 31, xo1 == 310), "")
INS(ecowx, (rd), (ra, rb), (), (opcd == 31, xo1 == 438), "")
*/

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
   default:
      xError() << "Invalid mfspr SPR " << static_cast<uint32_t>(spr);
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
   default:
      xError() << "Invalid mtspr SPR " << static_cast<uint32_t>(spr);
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
      xError() << "Invalid mftb TBR " << static_cast<uint32_t>(tbr);
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

// DODGY STUFF
using fptr_ret32_arg0 = uint32_t(*)();
using fptr_ret32_arg1 = uint32_t(*)(uint32_t);
using fptr_ret32_arg2 = uint32_t(*)(uint32_t,uint32_t);

// System call
static void
sc(ThreadState *state, Instruction instr)
{
   auto sym = state->module->symbols[instr.bd];
   auto fsym = reinterpret_cast<FunctionSymbol*>(sym);

   if (sym->type != SymbolInfo::Function) {
      xDebug() << "Attempted to call non-function symbol " << sym->name;
      return;
   }

   if (!fsym->systemFunction) {
      xDebug() << "Unimplemented system function " << sym->name;
      return;
   }
   
   // TODO: Resolve symbol to real kernel func
   auto func = reinterpret_cast<SystemFunction*>(fsym->systemFunction);

   uint64_t returnValue = 0;

   switch (func->arity) {
   case 0:
      returnValue = (*reinterpret_cast<fptr_ret32_arg0>(func->fptr))();
      break;
   case 1:
      returnValue = (*reinterpret_cast<fptr_ret32_arg1>(func->fptr))(state->gpr[3]);
      break;
   case 2:
      returnValue = (*reinterpret_cast<fptr_ret32_arg2>(func->fptr))(state->gpr[3], state->gpr[4]);
      break;
   }

   assert(func->returnBytes <= 8);

   if (func->returnBytes > 0) {
      state->gpr[3] = static_cast<uint32_t>(returnValue & make_bitmask(static_cast<uint32_t>(func->returnBytes / 8)));
   }
}

void
Interpreter::registerSystemInstructions()
{
   RegisterInstruction(mfspr);
   RegisterInstruction(mtspr);
   RegisterInstruction(mftb);
   RegisterInstruction(mfmsr);
   RegisterInstruction(mtmsr);
   RegisterInstruction(mfsr);
   RegisterInstruction(mfsrin);
   RegisterInstruction(mtsr);
   RegisterInstruction(mtsrin);
   RegisterInstruction(sc);
}
