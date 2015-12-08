#include <spdlog/details/format.h>
#include <gsl.h>
#include "latte_instructions.h"
#include "latte_shadir.h"
#include "latte_disassembler.h"

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

bool
disassembleCondition(State &state, shadir::CfInstruction *inst)
{
   if (inst->cond) {
      state.out << " CND(";

      switch (inst->cond) {
      case SQ_CF_COND_FALSE:
         state.out << "FALSE";
         break;
      case SQ_CF_COND_BOOL:
         state.out << "BOOL";
         break;
      case SQ_CF_COND_NOT_BOOL:
         state.out << "NOT_BOOL";
         break;
      }

      state.out << ") CF_CONST(" << inst->constant << ")";
   }

   return true;
}

static bool
disassembleLoop(State &state, shadir::CfInstruction *inst)
{
   switch (inst->id) {
   case SQ_CF_INST_LOOP_START:
   case SQ_CF_INST_LOOP_START_DX10:
   case SQ_CF_INST_LOOP_START_NO_AL:
      state.out << " FAIL_JUMP_ADDR(" << inst->addr << ")";
      increaseIndent(state);
      break;
   case SQ_CF_INST_LOOP_CONTINUE:
   case SQ_CF_INST_LOOP_BREAK:
      state.out << " ADDR(" << inst->addr << ")";
      break;
   case SQ_CF_INST_LOOP_END:
      state.out << " PASS_JUMP_ADDR(" << inst->addr << ")";
      decreaseIndent(state);
      break;
   default:
      state.out << " UNKNOWN_LOOP_CF_INST";
   }

   if (inst->popCount) {
      state.out << " POP_COUNT(" << inst->popCount << ")";
   }

   if (inst->validPixelMode) {
      state.out << " VALID_PIX";
   }

   if (!inst->barrier) {
      state.out << " NO_BARRIER";
   }

   state.out << '\n';
   return true;
}

static bool
disassembleJump(State &state, shadir::CfInstruction *inst)
{
   if (inst->id == SQ_CF_INST_CALL && inst->callCount) {
      state.out << " CALL_COUNT(" << inst->callCount << ")";
   }

   disassembleCondition(state, inst);

   if (inst->popCount) {
      state.out << " POP_COUNT(" << inst->popCount << ")";
   }

   if (inst->id == SQ_CF_INST_CALL || inst->id == SQ_CF_INST_ELSE || inst->id == SQ_CF_INST_JUMP) {
      state.out << " ADDR(" << inst->addr << ")";
   }

   if (inst->validPixelMode) {
      state.out << " VALID_PIX";
   }

   if (!inst->barrier) {
      state.out << " NO_BARRIER";
   }

   state.out << '\n';
   return true;
}

bool
disassembleControlFlow(State &state, shadir::CfInstruction *inst)
{
   state.out.write("{}{:02} {}", state.indent, inst->cfPC, inst->name);

   switch (inst->id) {
   case SQ_CF_INST_TEX:
      return disassembleTEX(state, inst);
   case SQ_CF_INST_VTX:
   case SQ_CF_INST_VTX_TC:
      return disassembleVTX(state, inst);
   case SQ_CF_INST_LOOP_START:
   case SQ_CF_INST_LOOP_START_DX10:
   case SQ_CF_INST_LOOP_START_NO_AL:
   case SQ_CF_INST_LOOP_END:
   case SQ_CF_INST_LOOP_CONTINUE:
   case SQ_CF_INST_LOOP_BREAK:
      return disassembleLoop(state, inst);
   case SQ_CF_INST_JUMP:
   case SQ_CF_INST_ELSE:
   case SQ_CF_INST_CALL:
   case SQ_CF_INST_CALL_FS:
   case SQ_CF_INST_RETURN:
   case SQ_CF_INST_POP_JUMP:
      return disassembleJump(state, inst);
   case SQ_CF_INST_EMIT_VERTEX:
   case SQ_CF_INST_EMIT_CUT_VERTEX:
   case SQ_CF_INST_CUT_VERTEX:
      if (!inst->barrier) {
         state.out << " NO_BARRIER";
      }

      state.out << '\n';
      break;
   case SQ_CF_INST_PUSH:
   case SQ_CF_INST_PUSH_ELSE:
   case SQ_CF_INST_KILL:
      disassembleCondition(state, inst);
      // pass through
   case SQ_CF_INST_POP:
   case SQ_CF_INST_POP_PUSH:
   case SQ_CF_INST_POP_PUSH_ELSE:
      if (inst->popCount) {
         state.out << " POP_COUNT(" << inst->popCount << ")";
      }

      if (inst->validPixelMode) {
         state.out << " VALID_PIX";
      }

      state.out << '\n';
      break;
   case SQ_CF_INST_END_PROGRAM:
   case SQ_CF_INST_NOP:
      state.out << '\n';
      break;
   case SQ_CF_INST_WAIT_ACK:
   case SQ_CF_INST_TEX_ACK:
   case SQ_CF_INST_VTX_ACK:
   case SQ_CF_INST_VTX_TC_ACK:
   default:
      state.out << " UNK_FORMAT\n";
      break;
   }

   return false;
}

} // namespace disassembler

bool
disassemble(Shader &shader, std::string &out)
{
   auto result = true;
   disassembler::State state;
   state.shader = &shader;

   for (auto &ins : shader.code) {
      switch (ins->type) {
      case shadir::Instruction::CF:
         result &= disassembler::disassembleControlFlow(state, reinterpret_cast<shadir::CfInstruction *>(ins.get()));
         break;
      case shadir::Instruction::CF_ALU:
         result &= disassembler::disassembleControlFlowALU(state, reinterpret_cast<shadir::CfAluInstruction *>(ins.get()));
         break;
      case shadir::Instruction::EXP:
         result &= disassembler::disassembleExport(state, reinterpret_cast<shadir::ExportInstruction *>(ins.get()));
         break;
      default:
         throw std::logic_error("Invalid top level instruction type");
      }

      state.out << "\n";
   }

   out = state.out.str();
   return result;
}

} // namespace latte
