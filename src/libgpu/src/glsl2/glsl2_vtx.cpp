#include "glsl2_translate.h"
#include "latte/latte_instructions.h"
#include <fmt/format.h>

using namespace latte;

/*
Unimplemented:
FETCH
SEMANTIC
BUFINFO
*/

namespace glsl2
{

static void
VTX_FETCH(State &state, const ControlFlowInst &cf, const VertexFetchInst &inst)
{
   //  FETCH R4.xyzw, R0.y, b131 NO_INDEX_OFFSET FMT_FROM_FETCH_CONSTANT MEGA(16) OFFSET(0)
   auto id = inst.word0.BUFFER_ID() + SQ_RES_OFFSET::VS_TEX_RESOURCE_0;

   // For now we only support reading from vertex buffers (uniform blocks)
   decaf_assert(id >= SQ_RES_OFFSET::VS_BUF_RESOURCE_0 && id < SQ_RES_OFFSET::VS_GSOUT_RESOURCE, fmt::format("Unsupported VTX_FETCH buffer id {}", id));

   // Let's only support a very expected set of values
   decaf_check(inst.word0.FETCH_TYPE() == SQ_VTX_FETCH_TYPE::NO_INDEX_OFFSET);
   decaf_check(inst.word1.USE_CONST_FIELDS() == 1);
   decaf_check(inst.word2.OFFSET() == 0);
   decaf_check(inst.word2.MEGA_FETCH() && (inst.word0.MEGA_FETCH_COUNT() + 1) == 16);

   auto dstSelX = inst.word1.DST_SEL_X();
   auto dstSelY = inst.word1.DST_SEL_Y();
   auto dstSelZ = inst.word1.DST_SEL_Z();
   auto dstSelW = inst.word1.DST_SEL_W();

   auto numDstSels = 4u;
   auto dstSelMask = condenseSelections(dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);

   if (numDstSels > 0) {
      auto dst = getExportRegister(inst.gpr.DST_GPR(), inst.gpr.DST_REL());
      auto src = getExportRegister(inst.word0.SRC_GPR(), inst.word0.SRC_REL());
      auto blockID = id - SQ_RES_OFFSET::VS_BUF_RESOURCE_0;

      if (state.shader) {
         state.shader->usedUniformBlocks[blockID] = true;
      }

      fmt::memory_buffer tmp;
      fmt::format_to(tmp, "UB_{}.values[floatBitsToInt(", blockID);
      insertSelectValue(tmp, src, inst.word0.SRC_SEL_X());
      fmt::format_to(tmp, ")]");

      insertLineStart(state);
      fmt::format_to(state.out, "{}.{} = ", dst, dstSelMask);
      insertSelectVector(state.out, to_string(tmp), dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);
      fmt::format_to(state.out, ";");
      insertLineEnd(state);
   }
}

void
registerVtxFunctions()
{
   registerInstruction(latte::SQ_VTX_INST_FETCH, VTX_FETCH);
}

} // namespace glsl2
