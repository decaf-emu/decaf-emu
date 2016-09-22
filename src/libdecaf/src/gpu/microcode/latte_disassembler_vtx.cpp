#include "latte_disassembler.h"
#include "common/decaf_assert.h"

namespace latte
{

namespace disassembler
{

void
disassembleVtxInstruction(fmt::MemoryWriter &out,
                          const latte::ControlFlowInst &parent,
                          const VertexFetchInst &vtx,
                          int namePad)
{
   auto id = vtx.word0.VTX_INST();
   auto name = getInstructionName(id);

   out << fmt::pad(name, namePad, ' ') << ' ';

   // dst
   auto dstSelX = vtx.word1.DST_SEL_X();
   auto dstSelY = vtx.word1.DST_SEL_Y();
   auto dstSelZ = vtx.word1.DST_SEL_Z();
   auto dstSelW = vtx.word1.DST_SEL_W();

   if (dstSelX != latte::SQ_SEL::SEL_MASK || dstSelY != latte::SQ_SEL::SEL_MASK || dstSelZ != latte::SQ_SEL::SEL_MASK || dstSelW != latte::SQ_SEL::SEL_MASK) {
      if (id == SQ_VTX_INST_SEMANTIC) {
         out << "SEM[" << vtx.sem.SEMANTIC_ID() << "]";
      } else {
         out << "R" << vtx.gpr.DST_GPR();

         if (vtx.gpr.DST_REL() == SQ_REL::REL) {
            out << "[AL]";
         }
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
   out << ", R" << vtx.word0.SRC_GPR();

   if (vtx.word0.SRC_REL() == SQ_REL::REL) {
      out << "[AL]";
   }

   out
      << '.'
      << disassembleDestMask(vtx.word0.SRC_SEL_X());

   out
      << ", b" << vtx.word0.BUFFER_ID();


   // fetch_type
   if (vtx.word0.FETCH_TYPE() == SQ_VTX_FETCH_TYPE::VERTEX_DATA) {
      out << " VERTEX_DATA";
   } else if (vtx.word0.FETCH_TYPE() == SQ_VTX_FETCH_TYPE::INSTANCE_DATA) {
      out << " INSTANCE_DATA";
   } else if(vtx.word0.FETCH_TYPE() == SQ_VTX_FETCH_TYPE::NO_INDEX_OFFSET) {
      out << " NO_INDEX_OFFSET";
   } else {
      out << " FETCH_TYPE(" << vtx.word0.FETCH_TYPE() << ")";
   }


   // format
   if (!vtx.word1.USE_CONST_FIELDS()) {
      out << " FORMAT(";

      out << " " << vtx.word1.DATA_FORMAT();

      if (vtx.word1.NUM_FORMAT_ALL() == 0) {
         out << " NORM";
      } else if (vtx.word1.NUM_FORMAT_ALL() == 1) {
         out << " INT";
      } else if (vtx.word1.NUM_FORMAT_ALL() == 2) {
         out << " SCALED";
      } else {
         out << vtx.word1.NUM_FORMAT_ALL();
      }

      if (vtx.word1.FORMAT_COMP_ALL()) {
         out << " SIGNED";
      } else {
         out << " UNSIGNED";
      }

      out << " " << vtx.word1.SRF_MODE_ALL();

      out << ")";
   } else {
      out << " FMT_FROM_FETCH_CONSTANT";
   }

   if (vtx.word2.MEGA_FETCH()) {
      out << " MEGA(" << (vtx.word0.MEGA_FETCH_COUNT() + 1) << ")";
   } else {
      out << " MINI(" << (vtx.word0.MEGA_FETCH_COUNT() + 1) << ")";
   }

   out << " OFFSET(" << vtx.word2.OFFSET() << ")";

   if (vtx.word0.FETCH_WHOLE_QUAD()) {
      out << " WHOLE_QUAD";
   }

   if (vtx.word2.ENDIAN_SWAP() == SQ_ENDIAN::SWAP_8IN16) {
      out << " ENDIAN_SWAP(8IN16)";
   } else if (vtx.word2.ENDIAN_SWAP() == SQ_ENDIAN::SWAP_8IN32) {
      out << " ENDIAN_SWAP(8IN32)";
   } else if (vtx.word2.ENDIAN_SWAP() != SQ_ENDIAN::NONE) {
      out << " ENDIAN_SWAP(" << vtx.word2.ENDIAN_SWAP() << ")";
   }

   if (vtx.word2.CONST_BUF_NO_STRIDE()) {
      out << " CONST_BUF_NO_STRIDE";
   }

   if (vtx.word2.ALT_CONST()) {
      out << " ALT_CONST";
   }
}

void
disassembleVtxClause(State &state, const latte::ControlFlowInst &inst)
{
   auto addr = inst.word0.ADDR;
   auto count = (inst.word1.COUNT() + 1) | (inst.word1.COUNT_3() << 3);
   auto clause = reinterpret_cast<const VertexFetchInst *>(state.binary.data() + 8 * addr);

   increaseIndent(state);

   for (auto i = 0u; i < count; ++i) {
      const auto &vtx = clause[i];

      state.out
         << '\n'
         << state.indent
         << fmt::pad(state.groupPC, 3, ' ')
         << "    ";

      disassembleVtxInstruction(state.out, inst, vtx, 15);
      state.out << "\n";
      state.groupPC++;
   }

   decreaseIndent(state);
}

void
disassembleCfVTX(fmt::MemoryWriter &out, const ControlFlowInst &inst)
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
}

} // namespace disassembler

} // namespace latte
