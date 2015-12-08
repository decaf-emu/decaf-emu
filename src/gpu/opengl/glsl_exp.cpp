#include "glsl_generator.h"

using latte::shadir::ExportInstruction;

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

namespace gpu
{

namespace opengl
{

namespace glsl
{

static bool
EXP(GenerateState &state, ExportInstruction *ins)
{
   assert(ins->wholeQuadMode == false);
   //assert(ins->barrier == true);
   assert(ins->index == 0);
   assert(ins->elemSize == 0);

   switch (ins->exportType) {
   case latte::SQ_EXPORT_POS:
      state.out
         << "exp_position_"
         << (ins->arrayBase - 60);
      break;
   case latte::SQ_EXPORT_PARAM:
      state.out
         << "exp_param_"
         << ins->arrayBase;
      break;
   case latte::SQ_EXPORT_PIXEL:
      state.out
         << "exp_pixel_"
         << ins->arrayBase;
      break;
   }

   state.out << " = R" << ins->rw.id;
   translateSelectMask(state, ins->srcSel, 4);
   return true;
}

void registerExp()
{
   registerGenerator(latte::SQ_CF_INST_EXP, EXP);
   registerGenerator(latte::SQ_CF_INST_EXP_DONE, EXP);
}

} // namespace glsl

} // namespace opengl

} // namespace gpu
