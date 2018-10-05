#include "latte/latte_disassembler.h"

#include <common/bitutils.h>
#include <common/decaf_assert.h>
#include <common/fixed.h>
#include <fmt/format.h>

namespace latte
{

namespace disassembler
{

void
disassembleTexInstruction(fmt::memory_buffer &out,
                          const latte::ControlFlowInst &parent,
                          const TextureFetchInst &tex,
                          int namePad)
{
   auto id = tex.word0.TEX_INST();
   auto name = getInstructionName(id);

   if (id == SQ_TEX_INST_VTX_FETCH || id == SQ_TEX_INST_VTX_SEMANTIC) {
      // This only works because the TEX instruction IDs precisely match the
      //  VTX instruction IDs.  Ensure that is the case for anything else
      //  that is added to this redirection code.
      auto vtx = *reinterpret_cast<const VertexFetchInst*>(&tex);
      disassembleVtxInstruction(out, parent, vtx, namePad);
      return;
   }

   fmt::format_to(out, "{: <{}} ", name, namePad);

   // dst
   auto dstSelX = tex.word1.DST_SEL_X();
   auto dstSelY = tex.word1.DST_SEL_Y();
   auto dstSelZ = tex.word1.DST_SEL_Z();
   auto dstSelW = tex.word1.DST_SEL_W();

   if (dstSelX != latte::SQ_SEL::SEL_MASK || dstSelY != latte::SQ_SEL::SEL_MASK || dstSelZ != latte::SQ_SEL::SEL_MASK || dstSelW != latte::SQ_SEL::SEL_MASK) {
      fmt::format_to(out, "R{}", tex.word1.DST_GPR());

      if (tex.word1.DST_REL() == SQ_REL::REL) {
         fmt::format_to(out, "[AL]");
      }

      fmt::format_to(out, ".{}{}{}{}",
         disassembleDestMask(dstSelX),
         disassembleDestMask(dstSelY),
         disassembleDestMask(dstSelZ),
         disassembleDestMask(dstSelW));
   } else {
      fmt::format_to(out, "____");
   }


   // src
   fmt::format_to(out, ", R{}", tex.word0.SRC_GPR());

   if (tex.word0.SRC_REL() == SQ_REL::REL) {
      fmt::format_to(out, "[AL]");
   }

   fmt::format_to(out, ".{}{}{}{}",
      disassembleDestMask(tex.word2.SRC_SEL_X()),
      disassembleDestMask(tex.word2.SRC_SEL_Y()),
      disassembleDestMask(tex.word2.SRC_SEL_Z()),
      disassembleDestMask(tex.word2.SRC_SEL_W()));

   fmt::format_to(out, ", t{}, s{}",
                  tex.word0.RESOURCE_ID(),
                  tex.word2.SAMPLER_ID());

   if (tex.word1.LOD_BIAS()) {
      fmt::format_to(out, " LOD({})", static_cast<float>(tex.word1.LOD_BIAS()));
   }

   if (tex.word0.FETCH_WHOLE_QUAD()) {
      fmt::format_to(out, " WHOLE_QUAD");
   }

   if (tex.word0.BC_FRAC_MODE()) {
      fmt::format_to(out, " BC_FRAC_MODE");
   }

   if (tex.word0.ALT_CONST()) {
      fmt::format_to(out, " ALT_CONST");
   }

   auto normX = tex.word1.COORD_TYPE_X();
   auto normY = tex.word1.COORD_TYPE_Y();
   auto normZ = tex.word1.COORD_TYPE_Z();
   auto normW = tex.word1.COORD_TYPE_W();

   if (!normX || !normY || !normZ || !normW) {
      fmt::format_to(out, " DENORM(");

      if (!normX) {
         fmt::format_to(out, "X");
      }

      if (!normY) {
         fmt::format_to(out, "Y");
      }

      if (!normZ) {
         fmt::format_to(out, "Z");
      }

      if (!normW) {
         fmt::format_to(out, "W");
      }

      fmt::format_to(out, ")");
   }

   if (tex.word2.OFFSET_X()) {
      fmt::format_to(out, " XOFFSET({})", static_cast<float>(tex.word2.OFFSET_X()));
   }

   if (tex.word2.OFFSET_Y()) {
      fmt::format_to(out, " YOFFSET({})", static_cast<float>(tex.word2.OFFSET_Y()));
   }

   if (tex.word2.OFFSET_Z()) {
      fmt::format_to(out, " ZOFFSET({})", static_cast<float>(tex.word2.OFFSET_Z()));
   }
}

void
disassembleTEXClause(State &state, const ControlFlowInst &inst)
{
   auto addr = inst.word0.ADDR();
   auto count = (inst.word1.COUNT() | (inst.word1.COUNT_3() << 3)) + 1;
   auto clause = reinterpret_cast<const TextureFetchInst *>(state.binary.data() + 8 * addr);

   increaseIndent(state);

   for (auto i = 0u; i < count; ++i) {
      const auto &tex = clause[i];
      auto id = tex.word0.TEX_INST();

      fmt::format_to(state.out, "\n{}{: <3}    ", state.indent, state.groupPC);

      if (id == SQ_TEX_INST_VTX_FETCH || id == SQ_TEX_INST_VTX_SEMANTIC) {
         // Someone at AMD must have been having a laugh when they designed this...
         auto vtx = *reinterpret_cast<const VertexFetchInst *>(&tex);
         disassembleVtxInstruction(state.out, inst, vtx, 15);
      } else {
         disassembleTexInstruction(state.out, inst, tex, 15);
      }

      fmt::format_to(state.out, "\n");
      state.groupPC++;
   }

   decreaseIndent(state);
}

void
disassembleCfTEX(fmt::memory_buffer &out, const ControlFlowInst &inst)
{
   auto addr = inst.word0.ADDR();
   auto count = (inst.word1.COUNT() | (inst.word1.COUNT_3() << 3)) + 1;

   fmt::format_to(out, ": ADDR({}) CNT({})", addr, count);

   if (!inst.word1.BARRIER()) {
      fmt::format_to(out, " NO_BARRIER");
   }

   disassembleCondition(out, inst);

   if (inst.word1.WHOLE_QUAD_MODE()) {
      fmt::format_to(out, " WHOLE_QUAD");
   }

   if (inst.word1.VALID_PIXEL_MODE()) {
      fmt::format_to(out, " VALID_PIX");
   }
}

} // namespace disassembler

} // namespace latte
