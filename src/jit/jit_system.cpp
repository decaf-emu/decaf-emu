#include <cassert>
#include "bitutils.h"
#include "jit.h"
#include "log.h"

static SprEncoding
decodeSPR(Instruction instr)
{
   return static_cast<SprEncoding>(instr.sprl | (instr.spru << 5));
}

static bool
mfspr(PPCEmuAssembler& a, Instruction instr)
{
   auto spr = decodeSPR(instr);
   switch (spr) {
   case SprEncoding::XER:
      a.mov(a.eax, a.ppcxer);
      break;
   case SprEncoding::LR:
      a.mov(a.eax, a.ppclr);
      break;
   case SprEncoding::CTR:
      a.mov(a.eax, a.ppcctr);
      break;
   case SprEncoding::GQR0:
      a.mov(a.eax, a.ppcgpr[0]);
      break;
   case SprEncoding::GQR1:
      a.mov(a.eax, a.ppcgpr[1]);
      break;
   case SprEncoding::GQR2:
      a.mov(a.eax, a.ppcgpr[2]);
      break;
   case SprEncoding::GQR3:
      a.mov(a.eax, a.ppcgpr[3]);
      break;
   case SprEncoding::GQR4:
      a.mov(a.eax, a.ppcgpr[4]);
      break;
   case SprEncoding::GQR5:
      a.mov(a.eax, a.ppcgpr[5]);
      break;
   case SprEncoding::GQR6:
      a.mov(a.eax, a.ppcgpr[6]);
      break;
   case SprEncoding::GQR7:
      a.mov(a.eax, a.ppcgpr[7]);
      break;
   default:
      xError() << "Invalid mfspr SPR " << static_cast<uint32_t>(spr);
   }

   a.mov(a.ppcgpr[instr.rD], a.eax);
   return true;
}

void
JitManager::registerSystemInstructions()
{
   RegisterInstruction(mfspr);
}
