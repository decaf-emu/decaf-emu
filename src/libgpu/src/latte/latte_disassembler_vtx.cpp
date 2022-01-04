#include "latte/latte_disassembler_state.h"

#include <common/decaf_assert.h>
#include <fmt/core.h>
#include <iterator>

namespace latte
{

namespace disassembler
{

void
disassembleVtxInstruction(fmt::memory_buffer &out,
                          const latte::ControlFlowInst &parent,
                          const VertexFetchInst &vtx,
                          int namePad)
{
   auto id = vtx.word0.VTX_INST();
   auto name = getInstructionName(id);

   fmt::format_to(std::back_inserter(out), "{: <{}} ", name, namePad);

   // dst
   auto dstSelX = vtx.word1.DST_SEL_X();
   auto dstSelY = vtx.word1.DST_SEL_Y();
   auto dstSelZ = vtx.word1.DST_SEL_Z();
   auto dstSelW = vtx.word1.DST_SEL_W();

   if (dstSelX != latte::SQ_SEL::SEL_MASK || dstSelY != latte::SQ_SEL::SEL_MASK || dstSelZ != latte::SQ_SEL::SEL_MASK || dstSelW != latte::SQ_SEL::SEL_MASK) {
      if (id == SQ_VTX_INST_SEMANTIC) {
         fmt::format_to(std::back_inserter(out), "SEM[{}]", vtx.sem.SEMANTIC_ID());
      } else {
         fmt::format_to(std::back_inserter(out), "R{}", vtx.gpr.DST_GPR());

         if (vtx.gpr.DST_REL() == SQ_REL::REL) {
            fmt::format_to(std::back_inserter(out), "[AL]");
         }
      }

      fmt::format_to(std::back_inserter(out), ".{}{}{}{}",
                     disassembleDestMask(dstSelX),
                     disassembleDestMask(dstSelY),
                     disassembleDestMask(dstSelZ),
                     disassembleDestMask(dstSelW));
   } else {
      fmt::format_to(std::back_inserter(out), "____");
   }


   // src
   fmt::format_to(std::back_inserter(out), ", R{}", vtx.word0.SRC_GPR());

   if (vtx.word0.SRC_REL() == SQ_REL::REL) {
      fmt::format_to(std::back_inserter(out), "[AL]");
   }

   fmt::format_to(std::back_inserter(out), ".{}, b{}",
                  disassembleDestMask(vtx.word0.SRC_SEL_X()),
                  vtx.word0.BUFFER_ID());


   // fetch_type
   fmt::format_to(std::back_inserter(out), " FETCH_TYPE(");
   if (vtx.word0.FETCH_TYPE() == SQ_VTX_FETCH_TYPE::VERTEX_DATA) {
      fmt::format_to(std::back_inserter(out), "VERTEX_DATA");
   } else if (vtx.word0.FETCH_TYPE() == SQ_VTX_FETCH_TYPE::INSTANCE_DATA) {
      fmt::format_to(std::back_inserter(out), "INSTANCE_DATA");
   } else if(vtx.word0.FETCH_TYPE() == SQ_VTX_FETCH_TYPE::NO_INDEX_OFFSET) {
      fmt::format_to(std::back_inserter(out), "NO_INDEX_OFFSET");
   } else {
      fmt::format_to(std::back_inserter(out), "{}", vtx.word0.FETCH_TYPE());
   }
   fmt::format_to(std::back_inserter(out), ")");


   // format
   if (!vtx.word1.USE_CONST_FIELDS()) {
      fmt::format_to(std::back_inserter(out), " FORMAT(");

      fmt::format_to(std::back_inserter(out), " {}", vtx.word1.DATA_FORMAT());

      if (vtx.word1.NUM_FORMAT_ALL() == 0) {
         fmt::format_to(std::back_inserter(out), " NORM");
      } else if (vtx.word1.NUM_FORMAT_ALL() == 1) {
         fmt::format_to(std::back_inserter(out), " INT");
      } else if (vtx.word1.NUM_FORMAT_ALL() == 2) {
         fmt::format_to(std::back_inserter(out), " SCALED");
      } else {
         fmt::format_to(std::back_inserter(out), "{}", vtx.word1.NUM_FORMAT_ALL());
      }

      if (vtx.word1.FORMAT_COMP_ALL()) {
         fmt::format_to(std::back_inserter(out), " SIGNED");
      } else {
         fmt::format_to(std::back_inserter(out), " UNSIGNED");
      }

      fmt::format_to(std::back_inserter(out), " {}", vtx.word1.SRF_MODE_ALL());

      fmt::format_to(std::back_inserter(out), ")");
   }

   if (vtx.word2.MEGA_FETCH()) {
      fmt::format_to(std::back_inserter(out), " MEGA({})", vtx.word0.MEGA_FETCH_COUNT() + 1);
   } else {
      fmt::format_to(std::back_inserter(out), " MINI({})", vtx.word0.MEGA_FETCH_COUNT() + 1);
   }

   fmt::format_to(std::back_inserter(out), " OFFSET({})", vtx.word2.OFFSET());

   if (vtx.word0.FETCH_WHOLE_QUAD()) {
      fmt::format_to(std::back_inserter(out), " WHOLE_QUAD");
   }

   if (vtx.word2.ENDIAN_SWAP() == SQ_ENDIAN::SWAP_8IN16) {
      fmt::format_to(std::back_inserter(out), " ENDIAN_SWAP(8IN16)");
   } else if (vtx.word2.ENDIAN_SWAP() == SQ_ENDIAN::SWAP_8IN32) {
      fmt::format_to(std::back_inserter(out), " ENDIAN_SWAP(8IN32)");
   } else if (vtx.word2.ENDIAN_SWAP() != SQ_ENDIAN::NONE) {
      fmt::format_to(std::back_inserter(out), " ENDIAN_SWAP({})", vtx.word2.ENDIAN_SWAP());
   }

   if (vtx.word2.CONST_BUF_NO_STRIDE()) {
      fmt::format_to(std::back_inserter(out), " CONST_BUF_NO_STRIDE");
   }

   if (vtx.word2.ALT_CONST()) {
      fmt::format_to(std::back_inserter(out), " ALT_CONST");
   }
}

void
disassembleVtxClause(State &state, const latte::ControlFlowInst &inst)
{
   auto addr = inst.word0.ADDR();
   auto count = (inst.word1.COUNT() | (inst.word1.COUNT_3() << 3)) + 1;
   auto clause = reinterpret_cast<const VertexFetchInst *>(state.binary.data() + 8 * addr);

   increaseIndent(state);

   for (auto i = 0u; i < count; ++i) {
      const auto &vtx = clause[i];

      fmt::format_to(std::back_inserter(state.out), "\n{}{: <3}    ", state.indent, state.groupPC);
      disassembleVtxInstruction(state.out, inst, vtx, 15);
      fmt::format_to(std::back_inserter(state.out), "\n");
      state.groupPC++;
   }

   decreaseIndent(state);
}

void
disassembleCfVTX(fmt::memory_buffer &out, const ControlFlowInst &inst)
{
   auto addr = inst.word0.ADDR();
   auto count = (inst.word1.COUNT() | (inst.word1.COUNT_3() << 3)) + 1;

   fmt::format_to(std::back_inserter(out), ": ADDR({}) CNT({})", addr, count);

   if (!inst.word1.BARRIER()) {
      fmt::format_to(std::back_inserter(out), " NO_BARRIER");
   }

   disassembleCondition(out, inst);
}

} // namespace disassembler

} // namespace latte
