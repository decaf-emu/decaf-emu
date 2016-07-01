#include "glsl2_translate.h"
#include "gpu/microcode/latte_instructions.h"

#pragma optimize("", off)

using namespace latte;

/*
Unimplemented:
VTX_FETCH
VTX_SEMANTIC
MEM
LD
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

namespace glsl2
{

static char
getSelectChannel(SQ_SEL sel)
{
   switch (sel) {
   case SQ_SEL_X:
      return 'x';
   case SQ_SEL_Y:
      return 'y';
   case SQ_SEL_Z:
      return 'z';
   case SQ_SEL_W:
      return 'w';
   default:
      throw std::logic_error("Unexpected SQ_SEL value");
   }
}

static unsigned
getSamplerArgCount(SamplerType type)
{
   switch (type) {
   case SamplerType::Sampler1D:
      return 1;
   case SamplerType::Sampler2D:
      return 2;
   case SamplerType::Sampler3D:
      return 3;
   case SamplerType::Sampler1DArray:
      return 1 + 1;
   case SamplerType::Sampler2DArray:
      return 2 + 1;
   case SamplerType::Sampler1DShadow:
      return 1 + 1;
   case SamplerType::Sampler2DShadow:
      return 2 + 1;
   case SamplerType::Sampler1DArrayShadow:
      return 1 + 1 + 1;
   case SamplerType::Sampler2DArrayShadow:
      return 2 + 1 + 1;
   case SamplerType::SamplerCube:
      return 3;
   case SamplerType::SamplerCubeArray:
      return 3 + 1;
   case SamplerType::SamplerCubeShadow:
      return 3 + 1;
   case SamplerType::Sampler2DRect:
   case SamplerType::SamplerBuffer:
   case SamplerType::Sampler2DMS:
   case SamplerType::Sampler2DMSArray:
   case SamplerType::Sampler2DRectShadow:
   case SamplerType::SamplerCubeArrayShadow:
   default:
      throw std::logic_error(fmt::format("Unsupported sampler type {}", static_cast<unsigned>(type)));
   }
}

static void
registerSamplerID(State &state, unsigned id)
{
   if (state.shader) {
      state.shader->samplerUsed[id] = true;
   }
}

static void
SAMPLE(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   auto dstSelX = inst.word1.DST_SEL_X();
   auto dstSelY = inst.word1.DST_SEL_Y();
   auto dstSelZ = inst.word1.DST_SEL_Z();
   auto dstSelW = inst.word1.DST_SEL_W();

   auto srcSelX = inst.word2.SRC_SEL_X();
   auto srcSelY = inst.word2.SRC_SEL_Y();
   auto srcSelZ = inst.word2.SRC_SEL_Z();
   auto srcSelW = inst.word2.SRC_SEL_W();

   auto resourceID = inst.word0.RESOURCE_ID();
   auto samplerID = inst.word2.SAMPLER_ID();
   auto samplerType = glsl2::SamplerType::Sampler2D;
   registerSamplerID(state, samplerID);

   if (state.shader) {
      samplerType = state.shader->samplers[samplerID];
   }

   if (resourceID != samplerID) {
      throw std::logic_error("Unsupported sample with RESOURCE_ID != SAMPLER_ID");
   }

   auto dst = getExportRegister(inst.word1.DST_GPR(), inst.word1.DST_REL());
   auto src = getExportRegister(inst.word0.SRC_GPR(), inst.word0.SRC_REL());
   auto dstSelMask = getSelectDestinationMask(dstSelX, dstSelY, dstSelZ, dstSelW);

   insertLineStart(state);
   state.out << dst << "." << dstSelMask;
   state.out << " = texture(sampler_" << samplerID << ", ";

   auto samplerElements = getSamplerArgCount(samplerType);

   if (samplerElements == 1) {
      insertSelectValue(state.out, src, srcSelX);
   } else {
      auto srcSelCount = 0u;
      state.out << "vec" << samplerElements << "(";

      if (srcSelCount < samplerElements && insertSelectValue(state.out, src, srcSelX)) {
         srcSelCount++;

         if (srcSelCount < samplerElements) {
            state.out << ", ";
         }
      }

      if (srcSelCount < samplerElements && insertSelectValue(state.out, src, srcSelY)) {
         srcSelCount++;

         if (srcSelCount < samplerElements) {
            state.out << ", ";
         }
      }

      if (srcSelCount < samplerElements && insertSelectValue(state.out, src, srcSelZ)) {
         srcSelCount++;

         if (srcSelCount < samplerElements) {
            state.out << ", ";
         }
      }

      if (srcSelCount < samplerElements && insertSelectValue(state.out, src, srcSelW)) {
         srcSelCount++;
      }

      state.out << ")";
   }

   state.out << ")." << dstSelMask  << ';';
   insertLineEnd(state);
}

void
registerTexFunctions()
{
   registerInstruction(SQ_TEX_INST_SAMPLE, SAMPLE);
}

} // namespace glsl2
