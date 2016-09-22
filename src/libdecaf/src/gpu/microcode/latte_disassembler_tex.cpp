#include "latte_disassembler.h"
#include "common/bitutils.h"
#include "common/decaf_assert.h"

namespace latte
{

namespace disassembler
{

void
disassembleTexInstruction(fmt::MemoryWriter &out,
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

   out << fmt::pad(name, namePad, ' ') << ' ';

   // dst
   auto dstSelX = tex.word1.DST_SEL_X();
   auto dstSelY = tex.word1.DST_SEL_Y();
   auto dstSelZ = tex.word1.DST_SEL_Z();
   auto dstSelW = tex.word1.DST_SEL_W();

   if (dstSelX != latte::SQ_SEL::SEL_MASK || dstSelY != latte::SQ_SEL::SEL_MASK || dstSelZ != latte::SQ_SEL::SEL_MASK || dstSelW != latte::SQ_SEL::SEL_MASK) {
      out << "R" << tex.word1.DST_GPR();

      if (tex.word1.DST_REL() == SQ_REL::REL) {
         out << "[AL]";
      }

      out
         << '.'
         << disassembleDestMask(dstSelX)
         << disassembleDestMask(dstSelY)
         << disassembleDestMask(dstSelZ)
         << disassembleDestMask(dstSelW);
   } else {
      out << "____";
   }


   // src
   out << ", R" << tex.word0.SRC_GPR();

   if (tex.word0.SRC_REL() == SQ_REL::REL) {
      out << "[AL]";
   }

   out
      << '.'
      << disassembleDestMask(tex.word2.SRC_SEL_X())
      << disassembleDestMask(tex.word2.SRC_SEL_Y())
      << disassembleDestMask(tex.word2.SRC_SEL_Z())
      << disassembleDestMask(tex.word2.SRC_SEL_W());

   out
      << ", t" << tex.word0.RESOURCE_ID()
      << ", s" << tex.word2.SAMPLER_ID();

   if (tex.word1.LOD_BIAS()) {
      out << " LOD(" << sign_extend<7>(tex.word1.LOD_BIAS()) << ")";
   }

   if (tex.word0.FETCH_WHOLE_QUAD()) {
      out << " WHOLE_QUAD";
   }

   if (tex.word0.BC_FRAC_MODE()) {
      out << " BC_FRAC_MODE";
   }

   if (tex.word0.ALT_CONST()) {
      out << " ALT_CONST";
   }

   auto normX = tex.word1.COORD_TYPE_X();
   auto normY = tex.word1.COORD_TYPE_Y();
   auto normZ = tex.word1.COORD_TYPE_Z();
   auto normW = tex.word1.COORD_TYPE_W();

   if (!normX || !normY || !normZ || !normW) {
      out << " DENORM(";

      if (!normX) {
         out << "X";
      }

      if (!normY) {
         out << "Y";
      }

      if (!normZ) {
         out << "Z";
      }

      if (!normW) {
         out << "W";
      }

      out << ")";
   }

   if (tex.word2.OFFSET_X()) {
      auto offset = sign_extend<5>(tex.word2.OFFSET_X());
      out << " OFFSETX(" << offset << ")";
   }

   if (tex.word2.OFFSET_Y()) {
      auto offset = sign_extend<5>(tex.word2.OFFSET_Y());
      out << " OFFSETY(" << offset << ")";
   }

   if (tex.word2.OFFSET_Z()) {
      auto offset = sign_extend<5>(tex.word2.OFFSET_Z());
      out << " OFFSETZ(" << offset << ")";
   }
}

void
disassembleTEXClause(State &state, const ControlFlowInst &inst)
{
   auto addr = inst.word0.ADDR;
   auto count = (inst.word1.COUNT() + 1) | (inst.word1.COUNT_3() << 3);
   auto clause = reinterpret_cast<const TextureFetchInst *>(state.binary.data() + 8 * addr);

   increaseIndent(state);

   for (auto i = 0u; i < count; ++i) {
      const auto &tex = clause[i];
      auto id = tex.word0.TEX_INST();

      state.out
         << '\n'
         << state.indent
         << fmt::pad(state.groupPC, 3, ' ')
         << "    ";

      if (id == SQ_TEX_INST_VTX_FETCH || id == SQ_TEX_INST_VTX_SEMANTIC) {
         // Someone at AMD must have been having a laugh when they designed this...
         auto vtx = *reinterpret_cast<const VertexFetchInst *>(&tex);
         disassembleVtxInstruction(state.out, inst, vtx, 15);
      } else {
         disassembleTexInstruction(state.out, inst, tex, 15);
      }

      state.out << "\n";
      state.groupPC++;
   }

   decreaseIndent(state);
}

void
disassembleCfTEX(fmt::MemoryWriter &out, const ControlFlowInst &inst)
{
   auto addr = inst.word0.ADDR;
   auto count = (inst.word1.COUNT() + 1) | (inst.word1.COUNT_3() << 3);

   out
      << " ADDR(" << addr << ")"
      << " CNT(" << count << ")";

   if (!inst.word1.BARRIER()) {
      out << " NO_BARRIER";
   }

   disassembleCondition(out, inst);

   if (inst.word1.WHOLE_QUAD_MODE()) {
      out << " WHOLE_QUAD";
   }

   if (inst.word1.VALID_PIXEL_MODE()) {
      out << " VALID_PIX";
   }
}

} // namespace disassembler

} // namespace latte
