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

void
translateSelectMask(GenerateState &state, const std::array<latte::SQ_SEL, 4> &sel, size_t maxSel)
{
   state.out << '.';

   for (auto i = 0u; i < sel.size() && i < maxSel; ++i) {
      switch (sel[i]) {
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
         state.out << '_';
         break;
      case latte::SQ_SEL_1:
      case latte::SQ_SEL_0:
         throw std::logic_error("Support for constant-value swizzles is not implemented");
      }
   }
}

void
translateRegisterFetch(GenerateState &state, uint32_t id, const std::array<latte::SQ_SEL, 4> &sel, size_t maxSel)
{
   state.out << '.';

   for (auto i = 0u; i < sel.size() && i < maxSel; ++i) {
      switch (sel[i]) {
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
         state.out << '_';
         break;
      case latte::SQ_SEL_1:
      case latte::SQ_SEL_0:
         throw std::logic_error("Support for constant-value swizzles is not implemented");
      }
   }
}

static size_t
getSamplerMaxSel(GenerateState &state, uint32_t samplerId)
{
   size_t maxSel = 2;
   auto samplerDim = state.shader->samplers[samplerId];
   switch (samplerDim) {
   case latte::SQ_TEX_DIM_1D:
      return 1;
   case latte::SQ_TEX_DIM_2D:
      return 2;
   case latte::SQ_TEX_DIM_2D_ARRAY:
      return 3;
   default:
      throw std::logic_error("Sampler dim is not supported for latte TEX sample.");
   }
}

//! Takes X.x__w = Y.xxxx; and collapses to X.xw = Y.xx;
static size_t
collapseSwizzle(std::array<latte::SQ_SEL, 4> &dstSel, std::array<latte::SQ_SEL, 4> &srcSel)
{
   size_t maxSel = 0;

   for (auto i = 0u; i < srcSel.size(); ++i) {
      switch (srcSel[i]) {
      case latte::SQ_SEL_MASK:
         break;
      case latte::SQ_SEL_X:
      case latte::SQ_SEL_Y:
      case latte::SQ_SEL_Z:
      case latte::SQ_SEL_W:
      case latte::SQ_SEL_1:
      case latte::SQ_SEL_0:
         dstSel[maxSel] = dstSel[i];
         srcSel[maxSel] = srcSel[i];
         maxSel++;
         break;
      }
   }

   for (auto i = maxSel; i < 4u; ++i) {
      dstSel[i] = latte::SQ_SEL_MASK;
      srcSel[i] = latte::SQ_SEL_MASK;
   }

   return maxSel;
}

static bool
SAMPLE(GenerateState &state, TextureFetchInstruction *ins)
{
   if (ins->resourceID != ins->samplerID) {
      throw std::logic_error("Expect resourceID == samplerID");
   }

   std::array<latte::SQ_SEL, 4> dstSel;
   dstSel[0] = latte::SQ_SEL_X;
   dstSel[1] = latte::SQ_SEL_Y;
   dstSel[2] = latte::SQ_SEL_Z;
   dstSel[3] = latte::SQ_SEL_W;
   std::array<latte::SQ_SEL, 4> srcSel = ins->dst.sel;

   size_t maxSel = collapseSwizzle(dstSel, srcSel);

   if (ins->dst.id >= latte::SQ_ALU_TMP_REGISTER_FIRST) {
      state.out << "T[" << ins->dst.id << "]";
   } else {
      state.out << "R[" << ins->dst.id << "]";
   }

   translateSelectMask(state, dstSel, maxSel);

   state.out
      << " = texture(sampler_"
      << ins->samplerID
      << ", ";

   // TODO: We need access to sampler DIM information to be able
   //  to select accurate maxSel number...  Additionally we need
   //  some plumbing that allows us to convert something like
   //  R0.xy0w into vec4(R0.xy, 0, w).  Keeping in mind that
   //  with a function like this, we should store the texture
   //  result in an intermediate to save calling it multiple times.
   //  A function for handling selecting and assigning is probably in order.
   if (ins->src.id >= latte::SQ_ALU_TMP_REGISTER_FIRST) {
      state.out << "T[" << ins->src.id << "]";
   } else {
      state.out << "R[" << ins->src.id << "]";
   }

   // Half-implement the above comments concerns...
   translateSelectMask(state, ins->src.sel, getSamplerMaxSel(state, ins->samplerID));

   state.out << ")";
   translateSelectMask(state, srcSel, maxSel);

   return true;
}

void registerTex()
{
   registerGenerator(latte::SQ_TEX_INST_SAMPLE, SAMPLE);
}

} // namespace glsl

} // namespace opengl

} // namespace gpu
