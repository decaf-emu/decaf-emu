#include "glsl_generator.h"

/*
Unimplemented:
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
*/

using latte::shadir::TextureFetchInstruction;
using latte::shadir::TextureFetchRegister;

namespace gpu
{

namespace opengl
{

namespace glsl
{

static bool
translateSelect(GenerateState &state, latte::SQ_SEL sel)
{
   switch (sel) {
   case latte::SQ_SEL_X:
      state.out << 'x';
      break;
   case latte::SQ_SEL_Y:
      state.out << 'y';
      break;
   case latte::SQ_SEL_Z:
      state.out << 'z';
      break;
   case latte::SQ_SEL_W:
      state.out << 'w';
      break;
   case latte::SQ_SEL_MASK:
   case latte::SQ_SEL_1:
   case latte::SQ_SEL_0:
   default:
      return false;
   }

   return true;
}

unsigned
translateSelectMask(GenerateState &state, const std::array<latte::SQ_SEL, 4> &sel, size_t maxSel)
{
   if (maxSel > 0 && sel[0] != latte::SQ_SEL_MASK) {
      state.out << '.';
   }

   for (auto i = 0u; i < sel.size(); ++i) {
      if (maxSel > i && !translateSelect(state, sel[i])) {
         return i;
      }
   }

   return 4;
}

static unsigned
translateTextureFetchRegister(GenerateState &state, const TextureFetchRegister &reg, size_t maxSel)
{
   state.out << 'R' << reg.id;
   return translateSelectMask(state, reg.sel, maxSel);
}

static void
translateTexRegisterChannels(GenerateState &state, unsigned channels)
{
   if (channels > 0) {
      state.out << ".x";
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

static bool
SAMPLE(GenerateState &state, TextureFetchInstruction *ins)
{
   auto channels = translateTextureFetchRegister(state, ins->dst, 4);

   if (ins->resourceID != ins->samplerID) {
      throw std::logic_error("Expect resourceID == samplerID");
   }

   state.out
      << " = texture(sampler_"
      << ins->samplerID
      << ", ";

   translateTextureFetchRegister(state, ins->src, 3);

   state.out << ")";
   translateTexRegisterChannels(state, channels);
   return true;
}

static bool
SAMPLE_C(GenerateState &state, TextureFetchInstruction *ins)
{
   auto channels = translateTextureFetchRegister(state, ins->dst, 4);

   if (ins->resourceID != ins->samplerID) {
      throw std::logic_error("Expect resourceID == samplerID");
   }

   state.out
      << " = texture(sampler_"
      << ins->samplerID
      << ", vec3(";

   translateTextureFetchRegister(state, ins->src, 3);

   state.out
      << ", R" << ins->src.id << ".w)";

   state.out << ")";
   translateTexRegisterChannels(state, channels);
   return true;
}

void registerTex()
{
   registerGenerator(latte::SQ_TEX_INST_SAMPLE, SAMPLE);
   registerGenerator(latte::SQ_TEX_INST_SAMPLE_C, SAMPLE_C);
}

} // namespace glsl

} // namespace opengl

} // namespace gpu
