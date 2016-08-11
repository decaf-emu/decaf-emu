#include "glsl2_translate.h"
#include "gpu/microcode/latte_instructions.h"

using namespace latte;

/*
Unimplemented:
VTX_FETCH
VTX_SEMANTIC
MEM
LD
GET_SAMPLE_INFO
GET_COMP_TEX_LOD
GET_GRADIENTS_H
GET_GRADIENTS_V
GET_LERP
KEEP_GRADIENTS
SET_GRADIENTS_H
SET_GRADIENTS_V
PASS
SAMPLE_LB
SAMPLE_G
SAMPLE_G_L
SAMPLE_G_LB
SAMPLE_G_LZ
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
      throw translate_exception(fmt::format("Unexpected SQ_SEL {}", sel));
   }
}

static unsigned
getSamplerArgCount(latte::SQ_TEX_DIM type, bool isShadowOp)
{
   switch (type) {
   case latte::SQ_TEX_DIM_1D:
      return 1 + (isShadowOp ? 1 : 0);
   case latte::SQ_TEX_DIM_2D:
      return 2 + (isShadowOp ? 1 : 0);
   case latte::SQ_TEX_DIM_3D:
      decaf_assert(!isShadowOp, "Shadow3D samplers have special semantics we don't yet support");
      return 3;
   case latte::SQ_TEX_DIM_1D_ARRAY:
      return 1 + 1 + (isShadowOp ? 1 : 0);
   case latte::SQ_TEX_DIM_2D_ARRAY:
      return 2 + 1 + (isShadowOp ? 1 : 0);
   case latte::SQ_TEX_DIM_CUBEMAP:
      return 3 + (isShadowOp ? 1 : 0);
   case latte::SQ_TEX_DIM_2D_MSAA:
   case latte::SQ_TEX_DIM_2D_ARRAY_MSAA:
   default:
      throw translate_exception(fmt::format("Unsupported sampler type {}", static_cast<unsigned>(type)));
   }
}

static SamplerUsage
registerSamplerID(State &state, unsigned id, bool isShadowOp)
{
   decaf_check(state.shader);

   auto &usage = state.shader->samplerUsage[id];

   if (!isShadowOp) {
      decaf_check(usage == SamplerUsage::Invalid || usage == SamplerUsage::Texture);
      usage = SamplerUsage::Texture;
   } else {
      decaf_check(usage == SamplerUsage::Invalid || usage == SamplerUsage::Shadow);
      usage = SamplerUsage::Shadow;
   }

   return usage;
}

static void
sampleFunc(State &state,
           const latte::ControlFlowInst &cf,
           const latte::TextureFetchInst &inst,
           const std::string &func,
           bool isShadowOp = false,
           latte::SQ_SEL extraArg = latte::SQ_SEL_MASK)
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

   auto samplerDim = state.shader->samplerDim[samplerID];
   auto samplerUsage = registerSamplerID(state, samplerID, isShadowOp);

   if (resourceID != samplerID) {
      throw translate_exception("Unsupported sample with RESOURCE_ID != SAMPLER_ID");
   }

   auto dst = getExportRegister(inst.word1.DST_GPR(), inst.word1.DST_REL());
   auto src = getExportRegister(inst.word0.SRC_GPR(), inst.word0.SRC_REL());

   auto numDstSels = 4u;
   auto dstSelMask = condenseSelections(dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);

   if (numDstSels > 0) {
      insertLineStart(state);

      auto samplerElements = getSamplerArgCount(samplerDim, isShadowOp);

      if (!isShadowOp) {
         state.out << "texTmp";
      } else {
         state.out << "texTmp.x";
      }

      state.out << " = " << func << "(sampler_" << samplerID << ", ";

      if (isShadowOp) {
         /* In r600 the .w channel holds the compare value whereas OpenGL
          * shadow samplers just expect it to be the last texture coordinate
          * so we must set the last channel to SQ_SEL_W
          */
         if (samplerElements == 2) {
            srcSelY = srcSelW;
         } else if (samplerElements == 3) {
            srcSelZ = srcSelW;
         } else if (samplerElements == 4) {
            // The value will already be in place
         } else {
            decaf_abort(fmt::format("Unexpected samplerElements {} for shadow sampler", samplerElements));
         }
      }

      insertSelectVector(state.out, src, srcSelX, srcSelY, srcSelZ, srcSelW, samplerElements);

      // TODO: Possible performance bottleneck; this could be improved by
      //  skipping the multiply for textures whose guest and host size are
      //  equal, though that requires including the equality test as part of
      //  the shader key.
      state.out << " * ";
      insertSelectVector(state.out,
                         fmt::format("texScale[{}]", resourceID),
                         latte::SQ_SEL_X,
                         latte::SQ_SEL_Y,
                         latte::SQ_SEL_Z,
                         latte::SQ_SEL_W,
                         samplerElements);

      switch (extraArg) {
      case latte::SQ_SEL_X:
         state.out << ", ";
         insertSelectValue(state.out, src, srcSelX);
         break;
      case latte::SQ_SEL_Y:
         state.out << ", ";
         insertSelectValue(state.out, src, srcSelY);
         break;
      case latte::SQ_SEL_Z:
         state.out << ", ";
         insertSelectValue(state.out, src, srcSelZ);
         break;
      case latte::SQ_SEL_W:
         state.out << ", ";
         insertSelectValue(state.out, src, srcSelW);
         break;
      case latte::SQ_SEL_0:
         state.out << ", 0";
         break;
      case latte::SQ_SEL_1:
         state.out << ", 1";
         break;
      }

      state.out << ");";
      insertLineEnd(state);

      insertLineStart(state);
      state.out << dst << "." << dstSelMask;
      state.out << " = ";
      insertSelectVector(state.out, "texTmp", dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);
      state.out << ";";
      insertLineEnd(state);
   }
}

static void
FETCH4(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   sampleFunc(state, cf, inst, "textureGather");
}

static void
GET_TEXTURE_INFO(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
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
   // SAMPLER_ID is a don't care in this instruction, but we ensure that
   //  textures and samplers use the same IDs, so we can safely use
   //  RESOURCE_ID as the sampler ID.
   auto samplerID = resourceID;

   auto samplerDim = state.shader->samplerDim[samplerID];

   auto dst = getExportRegister(inst.word1.DST_GPR(), inst.word1.DST_REL());
   auto src = getExportRegister(inst.word0.SRC_GPR(), inst.word0.SRC_REL());
   // TODO: Which source component is used to select the LoD?  Xenoblade has:
   //  GET_TEXTURE_INFO R6.xy__, R4.xx0x, t4, s0 (with R4.x = 0)
   auto srcSelLod = srcSelX;

   // GET_TEXTURE_INFO returns {width, height, depth, mipmap count}, but GLSL
   //  has separate functions for W/H/D and mipmap count, so we need to split
   //  this up into two operations.

   auto numDstSels = 3u;
   SQ_SEL dummy = SQ_SEL_MASK;
   auto dstSelMask = condenseSelections(dstSelX, dstSelY, dstSelZ, dummy, numDstSels);

   if (numDstSels > 0) {
      auto samplerElements = getSamplerArgCount(samplerDim, false);

      insertLineStart(state);
      state.out << "texTmp.xyz = intBitsToFloat(ivec3(textureSize(sampler_" << samplerID << ", floatBitsToInt(";
      insertSelectValue(state.out, src, srcSelLod);
      state.out << "))";
      for (auto i = samplerElements; i < 3; ++i) {
         state.out << ", 1";
      }
      state.out << "));";
      insertLineEnd(state);

      insertLineStart(state);
      state.out << dst << "." << dstSelMask;
      state.out << " = ";
      insertSelectVector(state.out, "texTmp", dstSelX, dstSelY, dstSelZ, SQ_SEL::SQ_SEL_MASK, numDstSels);
      state.out << ";";
      insertLineEnd(state);
   }

   if (dstSelW != SQ_SEL::SQ_SEL_MASK) {
      insertLineStart(state);
      insertSelectValue(state.out, dst, dstSelW);
      state.out << " = intBitsToFloat(textureQueryLevels(sampler_" << samplerID << "));";
      insertLineEnd(state);
   }
}

static void
SET_CUBEMAP_INDEX(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   // TODO: It is possible that we are supposed to somehow force
   //  a specific face to be used in spite of the coordinates.
}

static void
SAMPLE(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   sampleFunc(state, cf, inst, "texture");
}

static void
SAMPLE_C(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   sampleFunc(state, cf, inst, "texture", true);
}

static void
SAMPLE_L(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   // Sample with LOD srcW
   sampleFunc(state, cf, inst, "textureLod", false, latte::SQ_SEL_W);
}

static void
SAMPLE_LZ(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   // Sample with LOD Zero
   sampleFunc(state, cf, inst, "textureLod", false, latte::SQ_SEL_0);
}

void
registerTexFunctions()
{
   registerInstruction(SQ_TEX_INST_FETCH4, FETCH4);
   registerInstruction(SQ_TEX_INST_GET_TEXTURE_INFO, GET_TEXTURE_INFO);
   registerInstruction(SQ_TEX_INST_SET_CUBEMAP_INDEX, SET_CUBEMAP_INDEX);
   registerInstruction(SQ_TEX_INST_SAMPLE, SAMPLE);
   registerInstruction(SQ_TEX_INST_SAMPLE_C, SAMPLE_C);
   registerInstruction(SQ_TEX_INST_SAMPLE_L, SAMPLE_L);
   registerInstruction(SQ_TEX_INST_SAMPLE_LZ, SAMPLE_LZ);
}

} // namespace glsl2
