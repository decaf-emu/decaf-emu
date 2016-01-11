#include "glsl_generator.h"

using latte::shadir::AluInstruction;
using latte::shadir::AluReductionInstruction;

/*
Unimplemented:
CUBE
MAX4
*/

namespace gpu
{

namespace opengl
{

namespace glsl
{

static bool
DOT4(GenerateState &state, AluReductionInstruction *ins)
{
   // dst = dot(src0, src1)
   assert(ins->units[0] && ins->units[1] && ins->units[2] && ins->units[3]);
   AluInstruction *dest = nullptr;

   // Check if any units have a dest
   for (auto i = 0; i < 4; ++i) {
      if (ins->units[i]->writeMask) {
         assert(!dest);
         dest = ins->units[i];
      }
   }

   // If all units are masked we can pass unit0 to translateAluDest to generate the PV.x call
   if (!dest) {
      assert(!ins->units[0]->writeMask);
      assert(ins->units[0]->dst.chan == latte::SQ_CHAN_X);
      dest = ins->units[0];
   }

   translateAluDestStart(state, dest);

   state.out << "dot(";
   translateAluSourceVector(state,
                            ins->units[0],
                            ins->units[0]->src[0],
                            ins->units[1]->src[0],
                            ins->units[2]->src[0],
                            ins->units[3]->src[0]);
   state.out << ", ";
   translateAluSourceVector(state,
                            ins->units[0],
                            ins->units[0]->src[1],
                            ins->units[1]->src[1],
                            ins->units[2]->src[1],
                            ins->units[3]->src[1]);
   state.out << ')';

   translateAluDestEnd(state, dest);
   return true;
}

void registerAluReduction()
{
   registerGenerator(latte::SQ_OP2_INST_DOT4, DOT4);
   registerGenerator(latte::SQ_OP2_INST_DOT4_IEEE, DOT4);
}

} // namespace glsl

} // namespace opengl

} // namespace gpu
