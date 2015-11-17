#include "glsl_generator.h"

using latte::shadir::ExportInstruction;
using latte::shadir::SelRegister;

/*
Unimplemented:
MEM_STREAM0
MEM_STREAM1
MEM_STREAM2
MEM_STREAM3
MEM_SCRATCH
MEM_REDUCTION
MEM_RING
EXP
MEM_EXPORT
*/

namespace glsl
{

static bool EXP(GenerateState &state, ExportInstruction *ins)
{
   assert(ins->wholeQuadMode == false);
   //assert(ins->barrier == true);
   assert(ins->indexGpr == 0);
   assert(ins->elemSize == 0);

   switch (ins->type) {
   case latte::exp::Type::Position:
      state.out
         << "exp_position_"
         << ins->dstReg;
      break;
   case latte::exp::Type::Parameter:
      state.out
         << "exp_param_"
         << ins->dstReg;
      break;
   case latte::exp::Type::Pixel:
      state.out
         << "exp_color_"
         << ins->dstReg;
      break;
   }

   state.out << " = ";
   translateSelRegister(state, ins->src);
   return true;
}

void registerExp()
{
   using latte::exp::inst;
   registerGenerator(inst::EXP, EXP);
   registerGenerator(inst::EXP_DONE, EXP);
}

} // namespace hlsl
