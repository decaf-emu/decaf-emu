#include "hlsl_generator.h"

using latte::shadir::TexInstruction;
using latte::shadir::SelRegister;

/*
Unimplemented:
VTX_FETCH
VTX_SEMANTIC
MEM
GET_TEXTURE_INFO
GET_SAMPLE_INFO
GET_COMP_TEX_LOD
GET_GRADIENTS_H
GET_GRADIENTS_V
GET_LERP
KEEP_GRADIENTS
SET_GRADIENTS_H
SET_GRADIENTS_V
PASS
SET_CUBEMAP_INDEX
FETCH4
SAMPLE
SAMPLE_L
SAMPLE_LB
SAMPLE_LZ
SAMPLE_G
SAMPLE_G_L
SAMPLE_G_LB
SAMPLE_G_LZ
SAMPLE_C
SAMPLE_C_L
SAMPLE_C_LB
SAMPLE_C_LZ
SAMPLE_C_G
SAMPLE_C_G_L
SAMPLE_C_G_LB
SAMPLE_C_G_LZ
SET_TEXTURE_OFFSETS
GATHER4
GATHER4_O
GATHER4_C
GATHER4_C_O
GET_BUFFER_RESINFO
*/

namespace hlsl
{

static bool translateSelect(GenerateState &state, latte::alu::Select::Select sel)
{
   switch (sel) {
   case latte::alu::Select::X:
      state.out << 'x';
      break;
   case latte::alu::Select::Y:
      state.out << 'y';
      break;
   case latte::alu::Select::Z:
      state.out << 'z';
      break;
   case latte::alu::Select::W:
      state.out << 'w';
      break;
   case latte::alu::Select::Mask:
   case latte::alu::Select::One:
   case latte::alu::Select::Zero:
      return false;
   }

   return true;
}

unsigned translateSelRegister(GenerateState &state, SelRegister &reg)
{
   state.out << 'R' << reg.id;

   if (reg.selX != latte::alu::Select::Mask) {
      state.out << '.';
   }

   if (!translateSelect(state, reg.selX)) {
      return 0;
   }

   if (!translateSelect(state, reg.selY)) {
      return 1;
   }

   if (!translateSelect(state, reg.selZ)) {
      return 2;
   }

   if (!translateSelect(state, reg.selW)) {
      return 3;
   }

   return 4;
}

static void translateTexRegisterChannels(GenerateState &state, unsigned channels)
{
   if (channels > 0) {
      state.out << 'x';
   }

   if (channels > 1) {
      state.out << 'y';
   }

   if (channels > 2) {
      state.out << 'z';
   }

   if (channels > 3) {
      state.out << 'w';
   }
}

static bool SAMPLE(GenerateState &state, TexInstruction *ins)
{
   auto channels = translateSelRegister(state, ins->dst);

   state.out
      << " = g_texture"
      << ins->resourceID
      << ".Sample("
      << "g_sampler"
      << ins->samplerID
      << ", ";

   translateSelRegister(state, ins->src);

   state.out << ").";
   translateTexRegisterChannels(state, channels);
   return true;
}

   return true;
}

void registerTex()
{
   using latte::tex::inst;
   registerGenerator(inst::SAMPLE, SAMPLE);
}

} // namespace hlsl
