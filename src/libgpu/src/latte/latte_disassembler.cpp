#include "latte/latte_instructions.h"
#include "latte/latte_disassembler.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>
#include <gsl.h>

namespace latte
{

namespace disassembler
{

static const std::string
SingleIndent = "  ";

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
      decaf_abort("Invalid decrease indent");
   }
}

void
disassembleCondition(fmt::memory_buffer &out, const ControlFlowInst &inst)
{
   if (inst.word1.COND()) {
      fmt::format_to(out, " CND(");

      switch (inst.word1.COND()) {
      case SQ_CF_COND::ALWAYS_FALSE:
         fmt::format_to(out, "FALSE");
         break;
      case SQ_CF_COND::CF_BOOL:
         fmt::format_to(out, "BOOL");
         break;
      case SQ_CF_COND::CF_NOT_BOOL:
         fmt::format_to(out, "NOT_BOOL");
         break;
      }

      fmt::format_to(out, ") CF_CONST({})", inst.word1.CF_CONST());
   }
}

static void
disassembleLoop(fmt::memory_buffer &out, const ControlFlowInst &inst)
{
   disassembleCondition(out, inst);

   switch (inst.word1.CF_INST()) {
   case SQ_CF_INST_LOOP_START:
   case SQ_CF_INST_LOOP_END:
      if (!inst.word1.COND()) {
         // If .COND is set - disassembleCondition will print CF_CONST
         fmt::format_to(out, " CF_CONST({})", inst.word1.CF_CONST());
      }
   }

   switch (inst.word1.CF_INST()) {
   case SQ_CF_INST_LOOP_START:
   case SQ_CF_INST_LOOP_START_DX10:
   case SQ_CF_INST_LOOP_START_NO_AL:
      fmt::format_to(out, " FAIL_JUMP_ADDR({})", inst.word0.ADDR());
      break;
   case SQ_CF_INST_LOOP_CONTINUE:
   case SQ_CF_INST_LOOP_BREAK:
      fmt::format_to(out, " ADDR({})", inst.word0.ADDR());
      break;
   case SQ_CF_INST_LOOP_END:
      fmt::format_to(out, " PASS_JUMP_ADDR({})", inst.word0.ADDR());
      break;
   default:
      fmt::format_to(out, " UNKNOWN_LOOP_CF_INST");
   }

   if (inst.word1.POP_COUNT()) {
      fmt::format_to(out, " POP_CNT({})", inst.word1.POP_COUNT());
   }

   if (inst.word1.VALID_PIXEL_MODE()) {
      fmt::format_to(out, " VALID_PIX");
   }

   if (!inst.word1.BARRIER()) {
      fmt::format_to(out, " NO_BARRIER");
   }
}

static void
disassembleJump(fmt::memory_buffer &out, const ControlFlowInst &inst)
{
   auto id = inst.word1.CF_INST();

   if (id == SQ_CF_INST_CALL && inst.word1.CALL_COUNT()) {
      fmt::format_to(out, " CALL_COUNT({})", inst.word1.CALL_COUNT());
   }

   disassembleCondition(out, inst);

   if (inst.word1.POP_COUNT()) {
      fmt::format_to(out, " POP_CNT({})", inst.word1.POP_COUNT());
   }

   if (id == SQ_CF_INST_CALL || id == SQ_CF_INST_ELSE || id == SQ_CF_INST_JUMP) {
      fmt::format_to(out, " ADDR({})", inst.word0.ADDR());
   }

   if (inst.word1.VALID_PIXEL_MODE()) {
      fmt::format_to(out, " VALID_PIX");
   }

   if (!inst.word1.BARRIER()) {
      fmt::format_to(out, " NO_BARRIER");
   }
}

void
disassembleCF(fmt::memory_buffer &out, const ControlFlowInst &inst)
{
   auto id = inst.word1.CF_INST();
   auto name = getInstructionName(id);
   fmt::format_to(out, "{}", name);

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
         fmt::format_to(out, " NO_BARRIER");
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
         fmt::format_to(out, " POP_CNT({})", inst.word1.POP_COUNT());
      }

      if (inst.word1.VALID_PIXEL_MODE()) {
         fmt::format_to(out, " VALID_PIX");
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
      fmt::format_to(out, " UNK_FORMAT");
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
      decaf_abort(fmt::format("Unable to decode instruction {} {}", id, name));
   }

   // Decode instruction clause
   fmt::format_to(state.out, "{}{:02} ", state.indent, state.cfPC);
   disassembleCF(state.out, inst);
   fmt::format_to(state.out, "\n");

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
disassemble(const gsl::span<const uint8_t> &binary,
            bool isSubroutine)
{
   disassembler::State state;
   state.binary = binary;
   state.cfPC = 0;
   state.groupPC = 0;

   for (auto i = 0; i < binary.size(); i += sizeof(ControlFlowInst)) {
      auto cf = *reinterpret_cast<const ControlFlowInst *>(binary.data() + i);
      auto id = cf.word1.CF_INST();
      auto type = cf.word1.CF_INST_TYPE();

      switch (type) {
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
         decaf_abort(fmt::format("Invalid top level instruction type {}", type));
      }

      if (cf.word1.CF_INST() == SQ_CF_INST_RETURN && isSubroutine) {
         break;
      }

      if (cf.word1.CF_INST_TYPE() == SQ_CF_INST_TYPE_NORMAL
       || cf.word1.CF_INST_TYPE() == SQ_CF_INST_TYPE_EXPORT) {
         if (cf.word1.END_OF_PROGRAM()) {
            fmt::format_to(state.out, "\nEND_OF_PROGRAM\n");
            break;
         }
      }

      state.cfPC++;
      fmt::format_to(state.out, "\n");
   }

   return to_string(state.out);
}

} // namespace latte
