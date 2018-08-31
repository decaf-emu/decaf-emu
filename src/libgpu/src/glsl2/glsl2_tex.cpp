#include "glsl2_translate.h"
#include "latte/latte_instructions.h"
#include <fmt/format.h>

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
   case SQ_SEL::SEL_X:
      return 'x';
   case SQ_SEL::SEL_Y:
      return 'y';
   case SQ_SEL::SEL_Z:
      return 'z';
   case SQ_SEL::SEL_W:
      return 'w';
   default:
      throw translate_exception(fmt::format("Unexpected SQ_SEL {}", sel));
   }
}

static unsigned
getSamplerArgCount(latte::SQ_TEX_DIM type, bool isShadowOp)
{
   switch (type) {
   case latte::SQ_TEX_DIM::DIM_1D:
      return 1 + (isShadowOp ? 1 : 0);
   case latte::SQ_TEX_DIM::DIM_2D:
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      return 2 + (isShadowOp ? 1 : 0);
   case latte::SQ_TEX_DIM::DIM_3D:
      decaf_assert(!isShadowOp, "Shadow3D samplers have special semantics we don't yet support");
      return 3;
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      return 1 + 1 + (isShadowOp ? 1 : 0);
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      return 2 + 1 + (isShadowOp ? 1 : 0);
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      return 3 + (isShadowOp ? 1 : 0);
   default:
      throw translate_exception(fmt::format("Unsupported sampler type {}", static_cast<unsigned>(type)));
   }
}

static bool
getSamplerIsMsaa(latte::SQ_TEX_DIM type)
{
   switch (type) {
   case latte::SQ_TEX_DIM::DIM_1D:
   case latte::SQ_TEX_DIM::DIM_2D:
   case latte::SQ_TEX_DIM::DIM_3D:
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      return false;
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      return true;
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
           const std::string &offsetFunc,
           bool isShadowOp = false,
           latte::SQ_SEL extraArg = latte::SQ_SEL::SEL_MASK,
           bool asInts = false)
{
   auto dstSelX = inst.word1.DST_SEL_X();
   auto dstSelY = inst.word1.DST_SEL_Y();
   auto dstSelZ = inst.word1.DST_SEL_Z();
   auto dstSelW = inst.word1.DST_SEL_W();

   auto srcSelX = inst.word2.SRC_SEL_X();
   auto srcSelY = inst.word2.SRC_SEL_Y();
   auto srcSelZ = inst.word2.SRC_SEL_Z();
   auto srcSelW = inst.word2.SRC_SEL_W();

   auto offsetX = static_cast<float>(inst.word2.OFFSET_X());
   auto offsetY = static_cast<float>(inst.word2.OFFSET_Y());
   auto offsetZ = static_cast<float>(inst.word2.OFFSET_Z());

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
         fmt::format_to(state.out, "texTmp");
      } else {
         fmt::format_to(state.out, "texTmp.x");
      }

      fmt::format_to(state.out, " = ");

      bool writeOffsets = false;
      if (offsetX != 0 || offsetY != 0 || offsetZ != 0) {
         decaf_check(offsetFunc.size());
         fmt::format_to(state.out, "{}", offsetFunc);
         writeOffsets = true;
      } else {
         decaf_check(func.size());
         fmt::format_to(state.out, "{}", func);
      }

      fmt::format_to(state.out, "(sampler_{}, ", samplerID);

      if (isShadowOp) {
         /* In r600 the .w channel holds the compare value whereas OpenGL
          * shadow samplers just expect it to be the last texture coordinate
          * so we must set the last channel to SQ_SEL::SEL_W
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

      if (asInts) {
         fmt::format_to(state.out, "floatBitsToInt(");
      }

      insertSelectVector(state.out, src, srcSelX, srcSelY, srcSelZ, srcSelW, samplerElements);

      if (asInts) {
         fmt::format_to(state.out, ")");
      }

      switch (extraArg) {
      case latte::SQ_SEL::SEL_X:
         fmt::format_to(state.out, ", ");
         insertSelectValue(state.out, src, srcSelX);
         break;
      case latte::SQ_SEL::SEL_Y:
         fmt::format_to(state.out, ", ");
         insertSelectValue(state.out, src, srcSelY);
         break;
      case latte::SQ_SEL::SEL_Z:
         fmt::format_to(state.out, ", ");
         insertSelectValue(state.out, src, srcSelZ);
         break;
      case latte::SQ_SEL::SEL_W:
         fmt::format_to(state.out, ", ");
         insertSelectValue(state.out, src, srcSelW);
         break;
      case latte::SQ_SEL::SEL_0:
         fmt::format_to(state.out, ", 0");
         break;
      case latte::SQ_SEL::SEL_1:
         fmt::format_to(state.out, ", 1");
         break;
      }

      if (writeOffsets) {
         switch (samplerDim) {
         case latte::SQ_TEX_DIM::DIM_1D:
         case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
            fmt::format_to(state.out, ", {}", offsetX);
            break;
         case latte::SQ_TEX_DIM::DIM_2D:
         case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
         case latte::SQ_TEX_DIM::DIM_2D_MSAA:
         case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
            fmt::format_to(state.out, ", ivec2({}, {})", offsetX, offsetY);
            break;
         case latte::SQ_TEX_DIM::DIM_3D:
            fmt::format_to(state.out, ", ivec3({}, {}, {})", offsetX, offsetY, offsetZ);
            break;
         case latte::SQ_TEX_DIM::DIM_CUBEMAP:
         default:
            throw translate_exception(fmt::format("Unsupported sampler dim {}", static_cast<unsigned>(samplerDim)));
         }
      }

      if (getSamplerIsMsaa(samplerDim)) {
         // Write the sample number if this is an MSAA sampler
         fmt::format_to(state.out, ", 0");
      }

      fmt::format_to(state.out, ");");
      insertLineEnd(state);

      insertLineStart(state);
      fmt::format_to(state.out, "{}.{} = ", dst, dstSelMask);
      insertSelectVector(state.out, "texTmp", dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);
      fmt::format_to(state.out, ";");
      insertLineEnd(state);
   }
}

static void
FETCH4(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   sampleFunc(state, cf, inst, "textureGather", "textureGatherOffset");
}

static void
GET_GRADIENTS_H(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   auto resourceID = inst.word0.RESOURCE_ID();
   auto samplerID = inst.word2.SAMPLER_ID();
   decaf_check(resourceID == 0);
   decaf_check(samplerID == 0);

   auto dstSelX = inst.word1.DST_SEL_X();
   auto dstSelY = inst.word1.DST_SEL_Y();
   auto dstSelZ = inst.word1.DST_SEL_Z();
   auto dstSelW = inst.word1.DST_SEL_W();

   auto srcSelX = inst.word2.SRC_SEL_X();
   auto srcSelY = inst.word2.SRC_SEL_Y();
   auto srcSelZ = inst.word2.SRC_SEL_Z();
   auto srcSelW = inst.word2.SRC_SEL_W();

   auto numDstSels = 4u;
   auto dstSelMask = condenseSelections(dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);

   auto dst = getExportRegister(inst.word1.DST_GPR(), inst.word1.DST_REL());
   auto src = getExportRegister(inst.word0.SRC_GPR(), inst.word0.SRC_REL());

   if (numDstSels > 0) {
      insertLineStart(state);
      fmt::format_to(state.out, "{}.{} = dFdx(", dst, dstSelMask);
      insertSelectVector(state.out, src, srcSelX, srcSelY, srcSelZ, srcSelW, numDstSels);
      fmt::format_to(state.out, ");");
      insertLineEnd(state);
   }
}

static void
GET_GRADIENTS_V(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   auto resourceID = inst.word0.RESOURCE_ID();
   auto samplerID = inst.word2.SAMPLER_ID();
   decaf_check(resourceID == 0);
   decaf_check(samplerID == 0);

   auto dstSelX = inst.word1.DST_SEL_X();
   auto dstSelY = inst.word1.DST_SEL_Y();
   auto dstSelZ = inst.word1.DST_SEL_Z();
   auto dstSelW = inst.word1.DST_SEL_W();

   auto srcSelX = inst.word2.SRC_SEL_X();
   auto srcSelY = inst.word2.SRC_SEL_Y();
   auto srcSelZ = inst.word2.SRC_SEL_Z();
   auto srcSelW = inst.word2.SRC_SEL_W();

   auto numDstSels = 4u;
   auto dstSelMask = condenseSelections(dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);

   auto dst = getExportRegister(inst.word1.DST_GPR(), inst.word1.DST_REL());
   auto src = getExportRegister(inst.word0.SRC_GPR(), inst.word0.SRC_REL());

   if (numDstSels > 0) {
      insertLineStart(state);
      fmt::format_to(state.out, "{}.{} = dFdy(", dst, dstSelMask);
      insertSelectVector(state.out, src, srcSelX, srcSelY, srcSelZ, srcSelW, numDstSels);
      fmt::format_to(state.out, ");");
      insertLineEnd(state);
   }
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
   registerSamplerID(state, samplerID, false);

   auto dst = getExportRegister(inst.word1.DST_GPR(), inst.word1.DST_REL());
   auto src = getExportRegister(inst.word0.SRC_GPR(), inst.word0.SRC_REL());
   // TODO: Which source component is used to select the LoD?  Xenoblade has:
   //  GET_TEXTURE_INFO R6.xy__, R4.xx0x, t4, s0 (with R4.x = 0)
   auto srcSelLod = srcSelX;

   // GET_TEXTURE_INFO returns {width, height, depth, mipmap count}, but GLSL
   //  has separate functions for W/H/D and mipmap count, so we need to split
   //  this up into two operations.

   auto numDstSels = 3u;
   SQ_SEL dummy = SQ_SEL::SEL_MASK;
   auto dstSelMask = condenseSelections(dstSelX, dstSelY, dstSelZ, dummy, numDstSels);

   if (numDstSels > 0) {
      auto samplerElements = getSamplerArgCount(samplerDim, false);

      insertLineStart(state);
      fmt::format_to(state.out, "texTmp.xyz = intBitsToFloat(ivec3(textureSize(sampler_{}", samplerID);

      if (!getSamplerIsMsaa(samplerDim)) {
         fmt::format_to(state.out, ", floatBitsToInt(");
         insertSelectValue(state.out, src, srcSelLod);
         fmt::format_to(state.out, ")");
      }

      fmt::format_to(state.out, ")");
      for (auto i = samplerElements; i < 3; ++i) {
         fmt::format_to(state.out, ", 1");
      }
      fmt::format_to(state.out, "));");
      insertLineEnd(state);

      insertLineStart(state);
      fmt::format_to(state.out, "{}.{} = ", dst, dstSelMask);
      insertSelectVector(state.out, "texTmp", dstSelX, dstSelY, dstSelZ, SQ_SEL::SEL_MASK, numDstSels);
      fmt::format_to(state.out, ";");
      insertLineEnd(state);
   }

   if (dstSelW != SQ_SEL::SEL_MASK) {
      insertLineStart(state);
      insertSelectValue(state.out, dst, dstSelW);
      fmt::format_to(state.out, " = intBitsToFloat(textureQueryLevels(sampler_{}));", samplerID);
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
   sampleFunc(state, cf, inst, "texture", "textureOffset");
}

static void
SAMPLE_C(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   sampleFunc(state, cf, inst, "texture", "textureOffset", true);
}

static void
SAMPLE_L(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   // Sample with LOD srcW
   sampleFunc(state, cf, inst, "textureLod", "textureLodOffset", false, latte::SQ_SEL::SEL_W);
}

static void
SAMPLE_LZ(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   // Sample with LOD Zero
   sampleFunc(state, cf, inst, "textureLod", "textureLodOffset", false, latte::SQ_SEL::SEL_0);
}

static void
LD(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   // Texel Fetch
   sampleFunc(state, cf, inst, "texelFetch", "texelFetchOffset", false, latte::SQ_SEL::SEL_MASK, true);
}

void
registerTexFunctions()
{
   registerInstruction(SQ_TEX_INST_FETCH4, FETCH4);
   registerInstruction(SQ_TEX_INST_GET_GRADIENTS_H, GET_GRADIENTS_H);
   registerInstruction(SQ_TEX_INST_GET_GRADIENTS_V, GET_GRADIENTS_V);
   registerInstruction(SQ_TEX_INST_GET_TEXTURE_INFO, GET_TEXTURE_INFO);
   registerInstruction(SQ_TEX_INST_SET_CUBEMAP_INDEX, SET_CUBEMAP_INDEX);
   registerInstruction(SQ_TEX_INST_SAMPLE, SAMPLE);
   registerInstruction(SQ_TEX_INST_SAMPLE_C, SAMPLE_C);
   registerInstruction(SQ_TEX_INST_SAMPLE_L, SAMPLE_L);
   registerInstruction(SQ_TEX_INST_SAMPLE_LZ, SAMPLE_LZ);
   registerInstruction(SQ_TEX_INST_LD, LD);
}

} // namespace glsl2
