#include "latte_disassembler.h"
#include "common/decaf_assert.h"

namespace latte
{

namespace disassembler
{

void
disassembleVtxClause(State &state, const latte::ControlFlowInst &parent)
{
   auto addr = parent.word0.ADDR;
   auto count = (parent.word1.COUNT() + 1) | (parent.word1.COUNT_3() << 3);
   auto clause = reinterpret_cast<const TextureFetchInst *>(state.binary.data() + 8 * addr);

   increaseIndent(state);

   for (auto i = 0u; i < count; ++i) {
      const auto &tex = clause[i];
      auto id = tex.word0.TEX_INST();
      auto name = getInstructionName(id);

      if (id == SQ_TEX_INST_MEM) {
         decaf_abort(fmt::format("Unexpected mem instruction in vertex fetch clause {} {}", id, name));
      } else if (id != SQ_TEX_INST_VTX_FETCH && id != SQ_TEX_INST_VTX_SEMANTIC && id != SQ_TEX_INST_GET_BUFFER_RESINFO) {
         decaf_abort(fmt::format("Unexpected tex instruction in vertex fetch clause {} {}", id, name));
      }

      state.out
         << '\n'
         << state.indent
         << fmt::pad(state.groupPC, 3, ' ')
         << ' '
         << fmt::pad(name, 16, ' ')
         << " VTX_UNKNOWN_FORMAT";

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
