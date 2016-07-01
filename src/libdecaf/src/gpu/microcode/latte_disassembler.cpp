#include "latte_instructions.h"
#include "latte_disassembler.h"
#include <gsl.h>
#include <spdlog/details/format.h>

namespace latte
{

namespace disassembler
{

static std::string SingleIndent = "  ";

void
increaseIndent(State &state)
{
   state.indent += SingleIndent;
}

void
decreaseIndent(State &state)
{
   if (state.indent.size() >= SingleIndent.size()) {
      state.indent.resize(state.indent.size() - SingleIndent.size());
   } else {
      throw std::logic_error("Invalid decrease indent");
   }
}

void
disassembleCondition(fmt::MemoryWriter &out, const ControlFlowInst &inst)
{
   if (inst.word1.COND()) {
      out << " CND(";

      switch (inst.word1.COND()) {
      case SQ_CF_COND_FALSE:
         out << "FALSE";
         break;
      case SQ_CF_COND_BOOL:
         out << "BOOL";
         break;
      case SQ_CF_COND_NOT_BOOL:
         out << "NOT_BOOL";
         break;
      }

      out << ") CF_CONST(" << inst.word1.CF_CONST() << ")";
   }
}

static void
disassembleLoop(fmt::MemoryWriter &out, const ControlFlowInst &inst)
{
   switch (inst.word1.CF_INST()) {
   case SQ_CF_INST_LOOP_START:
   case SQ_CF_INST_LOOP_START_DX10:
   case SQ_CF_INST_LOOP_START_NO_AL:
      out << " FAIL_JUMP_ADDR(" << inst.word0.ADDR << ")";
      break;
   case SQ_CF_INST_LOOP_CONTINUE:
   case SQ_CF_INST_LOOP_BREAK:
      out << " ADDR(" << inst.word0.ADDR << ")";
      break;
   case SQ_CF_INST_LOOP_END:
      out << " PASS_JUMP_ADDR(" << inst.word0.ADDR << ")";
      break;
   default:
      out << " UNKNOWN_LOOP_CF_INST";
   }

   if (inst.word1.POP_COUNT()) {
      out << " POP_COUNT(" << inst.word1.POP_COUNT() << ")";
   }

   if (inst.word1.VALID_PIXEL_MODE()) {
      out << " VALID_PIX";
   }

   if (!inst.word1.BARRIER()) {
      out << " NO_BARRIER";
   }
}

static void
disassembleJump(fmt::MemoryWriter &out, const ControlFlowInst &inst)
{
   auto id = inst.word1.CF_INST();

   if (id == SQ_CF_INST_CALL && inst.word1.CALL_COUNT()) {
      out << " CALL_COUNT(" << inst.word1.CALL_COUNT() << ")";
   }

   disassembleCondition(out, inst);

   if (inst.word1.POP_COUNT()) {
      out << " POP_COUNT(" << inst.word1.POP_COUNT() << ")";
   }

   if (id == SQ_CF_INST_CALL || id == SQ_CF_INST_ELSE || id == SQ_CF_INST_JUMP) {
      out << " ADDR(" << inst.word0.ADDR << ")";
   }

   if (inst.word1.VALID_PIXEL_MODE()) {
      out << " VALID_PIX";
   }

   if (!inst.word1.BARRIER()) {
      out << " NO_BARRIER";
   }
}

void
disassembleCF(fmt::MemoryWriter &out, const ControlFlowInst &inst)
{
   auto id = inst.word1.CF_INST();
   auto name = getInstructionName(id);
   out << name;

   switch (id) {
   case SQ_CF_INST_TEX:
      disassembleCfTEX(out, inst);
      break;
   case SQ_CF_INST_VTX:
   case SQ_CF_INST_VTX_TC:
      disassembleCfVTX(out, inst);
      break;
   case SQ_CF_INST_LOOP_START:
   case SQ_CF_INST_LOOP_START_DX10:
   case SQ_CF_INST_LOOP_START_NO_AL:
   case SQ_CF_INST_LOOP_END:
   case SQ_CF_INST_LOOP_CONTINUE:
   case SQ_CF_INST_LOOP_BREAK:
      disassembleLoop(out, inst);
      break;
   case SQ_CF_INST_JUMP:
   case SQ_CF_INST_ELSE:
   case SQ_CF_INST_CALL:
   case SQ_CF_INST_CALL_FS:
   case SQ_CF_INST_RETURN:
   case SQ_CF_INST_POP_JUMP:
      disassembleJump(out, inst);
      break;
   case SQ_CF_INST_EMIT_VERTEX:
   case SQ_CF_INST_EMIT_CUT_VERTEX:
   case SQ_CF_INST_CUT_VERTEX:
      if (!inst.word1.BARRIER()) {
         out << " NO_BARRIER";
      }
      break;
   case SQ_CF_INST_PUSH:
   case SQ_CF_INST_PUSH_ELSE:
   case SQ_CF_INST_KILL:
      disassembleCondition(out, inst);
      // switch case pass through
   case SQ_CF_INST_POP:
   case SQ_CF_INST_POP_PUSH:
   case SQ_CF_INST_POP_PUSH_ELSE:
      if (inst.word1.POP_COUNT()) {
         out << " POP_COUNT(" << inst.word1.POP_COUNT() << ")";
      }

      if (inst.word1.VALID_PIXEL_MODE()) {
         out << " VALID_PIX";
      }
      break;
   case SQ_CF_INST_END_PROGRAM:
   case SQ_CF_INST_NOP:
      break;
   case SQ_CF_INST_WAIT_ACK:
   case SQ_CF_INST_TEX_ACK:
   case SQ_CF_INST_VTX_ACK:
   case SQ_CF_INST_VTX_TC_ACK:
   default:
      out << " UNK_FORMAT";
      break;
   }
}

static void
disassembleNormal(State &state, const ControlFlowInst &inst)
{
   auto id = inst.word1.CF_INST();
   auto name = getInstructionName(id);

   switch (id) {
   case SQ_CF_INST_WAIT_ACK:
   case SQ_CF_INST_TEX_ACK:
   case SQ_CF_INST_VTX_ACK:
   case SQ_CF_INST_VTX_TC_ACK:
      throw std::logic_error(fmt::format("Unable to decode instruction {} {}", id, name));
   }

   // Decode instruction clause
   state.out.write("{}{:02} ", state.indent, state.cfPC);
   disassembleCF(state.out, inst);
   state.out << "\n";

   switch (id) {
   case SQ_CF_INST_LOOP_START:
   case SQ_CF_INST_LOOP_START_DX10:
   case SQ_CF_INST_LOOP_START_NO_AL:
      increaseIndent(state);
      break;
   case SQ_CF_INST_LOOP_END:
      decreaseIndent(state);
      break;
   case SQ_CF_INST_TEX:
      disassembleTEXClause(state, inst);
      break;
   case SQ_CF_INST_VTX:
   case SQ_CF_INST_VTX_TC:
      disassembleVtxClause(state, inst);
      break;
   }
}

} // namespace disassembler

std::string
disassemble(const gsl::span<const uint8_t> &binary)
{
   disassembler::State state;
   state.binary = binary;
   state.cfPC = 0;
   state.groupPC = 0;

   for (auto i = 0; i < binary.size(); i += sizeof(ControlFlowInst)) {
      auto cf = *reinterpret_cast<const ControlFlowInst *>(binary.data() + i);
      auto id = cf.word1.CF_INST();

      switch (cf.word1.CF_INST_TYPE().get()) {
      case SQ_CF_INST_TYPE_NORMAL:
         disassembler::disassembleNormal(state, cf);
         break;
      case SQ_CF_INST_TYPE_EXPORT:
         disassembler::disassembleExport(state, cf);
         break;
      case SQ_CF_INST_TYPE_ALU:
      case SQ_CF_INST_TYPE_ALU_EXTENDED:
         disassembler::disassembleControlFlowALU(state, cf);
         break;
      default:
         throw std::logic_error("Invalid top level instruction type");
      }

      if (cf.word1.CF_INST_TYPE() == SQ_CF_INST_TYPE_NORMAL
       || cf.word1.CF_INST_TYPE() == SQ_CF_INST_TYPE_EXPORT) {
         if (cf.word1.END_OF_PROGRAM()) {
            break;
         }
      }

      state.cfPC++;
      state.out << "\n";
   }

   return state.out.str();
}

} // namespace latte
