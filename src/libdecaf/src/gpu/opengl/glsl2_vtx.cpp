#include "glsl2_translate.h"
#include "gpu/microcode/latte_instructions.h"

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
   auto id = inst.word0.BUFFER_ID().get() + SQ_VS_RESOURCE_BASE;

   // For now we only support reading from vertex buffers (uniform blocks)
   decaf_assert(id >= SQ_VS_BUF_RESOURCE_0 && id < SQ_VS_GSOUT_RESOURCE, fmt::format("Unsupported VTX_FETCH buffer id {}", id));

   // Let's only support a very expected set of values
   decaf_check(inst.word0.FETCH_TYPE() == SQ_VTX_FETCH_NO_INDEX_OFFSET);
   decaf_check(inst.word1.USE_CONST_FIELDS() == 1);
   decaf_check(inst.word2.OFFSET() == 0);
   decaf_check(inst.word2.MEGA_FETCH() && (inst.word0.MEGA_FETCH_COUNT() + 1) == 16);

   auto dstSelX = inst.word1.DST_SEL_X().get();
   auto dstSelY = inst.word1.DST_SEL_Y().get();
   auto dstSelZ = inst.word1.DST_SEL_Z().get();
   auto dstSelW = inst.word1.DST_SEL_W().get();

   auto numDstSels = 4u;
   auto dstSelMask = condenseSelections(dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);

   if (numDstSels > 0) {
      auto dst = getExportRegister(inst.gpr.DST_GPR(), inst.gpr.DST_REL());
      auto src = getExportRegister(inst.word0.SRC_GPR(), inst.word0.SRC_REL());
      inst.word0.SRC_SEL_X();

      auto blockID = id - SQ_VS_BUF_RESOURCE_0;

      if (state.shader) {
         state.shader->usedUniformBlocks[blockID] = true;
      }

      fmt::MemoryWriter tmp;
      tmp << "UB_" << blockID << ".values[floatBitsToInt(";
      insertSelectValue(tmp, src, inst.word0.SRC_SEL_X());
      tmp << ")]";

      insertLineStart(state);
      state.out << dst << "." << dstSelMask << " = ";
      insertSelectVector(state.out, tmp.str(), dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);
      state.out << ";";
      insertLineEnd(state);
   }
}

void
registerVtxFunctions()
{
   registerInstruction(latte::SQ_VTX_INST_FETCH, VTX_FETCH);
}

} // namespace glsl2
