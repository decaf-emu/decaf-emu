#define NOMINMAX
#include <spdlog/spdlog.h>
#include "latte.h"

static const auto indentSize = 3u;

namespace latte
{

struct GenerateState
{
   fmt::MemoryWriter out;
   std::string indent;
};

static bool translateBlock(GenerateState &state, shadir::Block *block);
static bool translateBlocks(GenerateState &state, std::vector<shadir::Block *> blocks);
static bool translateCodeBlock(GenerateState &state, shadir::CodeBlock *block);
static bool translateConditionalBlock(GenerateState &state, shadir::ConditionalBlock *block);
static bool translateLoopBlock(GenerateState &state, shadir::LoopBlock *block);
static bool translateInstruction(GenerateState &state, shadir::Instruction *ins);

static void beginLine(GenerateState &state)
{
   state.out << state.indent.c_str();
}

static void endLine(GenerateState &state)
{
   state.out << '\n';
}

static void increaseIndent(GenerateState &state)
{
   state.indent.append(indentSize, ' ');
}

static void decreaseIndent(GenerateState &state)
{
   if (state.indent.size() >= indentSize) {
      state.indent.resize(state.indent.size() - indentSize);
   }
}

using TranslateFuncCF = bool(*)(GenerateState &state, shadir::CfInstruction *ins);
using TranslateFuncALU = bool(*)(GenerateState &state, shadir::AluInstruction *ins);
using TranslateFuncTEX = bool(*)(GenerateState &state, shadir::TexInstruction *ins);

std::map<uint32_t, TranslateFuncCF> insTableCf;
std::map<uint32_t, TranslateFuncALU> insTableAluOp2;
std::map<uint32_t, TranslateFuncALU> insTableAluOp3;
std::map<uint32_t, TranslateFuncTEX> insTableTex;

static void translateChannel(GenerateState &state, latte::alu::Channel::Channel channel)
{
   switch (channel) {
   case latte::alu::Channel::X:
      state.out << 'x';
      break;
   case latte::alu::Channel::Y:
      state.out << 'y';
      break;
   case latte::alu::Channel::Z:
      state.out << 'z';
      break;
   case latte::alu::Channel::W:
      state.out << 'w';
      break;
   }
}

static void translateAluDestStart(GenerateState &state, shadir::AluInstruction *ins)
{
   if (ins->unit != shadir::AluInstruction::T) {
      state.out << "PV" << '.';
      translateChannel(state, static_cast<latte::alu::Channel::Channel>(ins->unit));
   } else {
      state.out << "PS";
   }

   state.out << " = ";

   if (ins->writeMask) {
      state.out << 'R' << ins->dest.id << '.';
      translateChannel(state, ins->dest.chan);
      state.out << " = ";
   }

   switch (ins->outputModifier) {
   case latte::alu::OutputModifier::Multiply2:
   case latte::alu::OutputModifier::Multiply4:
   case latte::alu::OutputModifier::Divide2:
      state.out << '(';
      break;
   }

   if (ins->dest.clamp) {
      state.out << "clamp(";
   }
}

static void translateAluDestEnd(GenerateState &state, shadir::AluInstruction *ins)
{
   if (ins->dest.clamp) {
      state.out << ", 0, 1)";
   }

   switch (ins->outputModifier) {
   case latte::alu::OutputModifier::Multiply2:
      state.out << ") * 2";
      break;
   case latte::alu::OutputModifier::Multiply4:
      state.out << ") * 4";
      break;
   case latte::alu::OutputModifier::Divide2:
      state.out << ") / 2";
      break;
   }
}

static void translateAluSource(GenerateState &state, shadir::AluSource &src)
{
   if (src.absolute) {
      state.out << "abs(";
   }

   if (src.negate) {
      state.out << '-';
   }

   switch (src.type) {
   case shadir::AluSource::Register:
      state.out << 'R' << src.id;
      break;
   case shadir::AluSource::KcacheBank0:
      state.out << "KcackeBank0_" << src.id;
      break;
   case shadir::AluSource::KcacheBank1:
      state.out << "KcackeBank1_" << src.id;
      break;
   case shadir::AluSource::PreviousVector:
      state.out << "PV";
      break;
   case shadir::AluSource::PreviousScalar:
      state.out << "PS";
      break;
   case shadir::AluSource::ConstantFile:
      state.out << 'C' << src.id;
      break;
   case shadir::AluSource::ConstantFloat:
      state.out.write("{:+f}f", src.floatValue);
      break;
   case shadir::AluSource::ConstantDouble:
      state.out.write("{:+f}", src.doubleValue);
      break;
   case shadir::AluSource::ConstantInt:
      state.out << src.intValue;
      break;
   }

   switch (src.type) {
   case shadir::AluSource::Register:
   case shadir::AluSource::KcacheBank0:
   case shadir::AluSource::KcacheBank1:
   case shadir::AluSource::ConstantFile:
   case shadir::AluSource::PreviousVector:
      state.out << '.';
      translateChannel(state, src.chan);
   }

   if (src.absolute) {
      state.out << ")";
   }
}

static bool translateCfLOOP_BREAK(GenerateState &state, shadir::CfInstruction *ins)
{
   state.out << "break;";
   return true;
}

static bool translateCfLOOP_CONTINUE(GenerateState &state, shadir::CfInstruction *ins)
{
   state.out << "continue;";
   return true;
}

static bool translateAluADD(GenerateState &state, shadir::AluInstruction *ins)
{
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " + ";
   translateAluSource(state, ins->sources[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool translateAluMOV(GenerateState &state, shadir::AluInstruction *ins)
{
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool translateAluFLT_TO_INT(GenerateState &state, shadir::AluInstruction *ins)
{
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "asint(";
   translateAluSource(state, ins->sources[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool translateAluPRED_SETE_INT(GenerateState &state, shadir::AluInstruction *ins)
{
   assert(ins->numSources == 2);

   if (!ins->writeMask && ins->updateExecutionMask && ins->updatePredicate) {
      translateAluSource(state, ins->sources[0]);
      state.out << " == ";
      translateAluSource(state, ins->sources[1]);
   } else {
      assert(false);
   }

   return true;
}

static bool translateAluPRED_SETNE_INT(GenerateState &state, shadir::AluInstruction *ins)
{
   assert(ins->numSources == 2);

   if (!ins->writeMask && ins->updateExecutionMask && ins->updatePredicate) {
      translateAluSource(state, ins->sources[0]);
      state.out << " != ";
      translateAluSource(state, ins->sources[1]);
   } else {
      assert(false);
   }

   return true;
}

static bool translateAluRECIP_IEEE(GenerateState &state, shadir::AluInstruction *ins)
{
   assert(ins->numSources == 1);
   translateAluDestStart(state, ins);

   state.out << "rcp(";
   translateAluSource(state, ins->sources[0]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool translateAluSETGE_DX10(GenerateState &state, shadir::AluInstruction *ins)
{
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   state.out << "(";
   translateAluSource(state, ins->sources[0]);
   state.out << " >= ";
   translateAluSource(state, ins->sources[1]);
   state.out << ")";

   translateAluDestEnd(state, ins);
   return true;
}

static bool translateAluMUL_IEEE(GenerateState &state, shadir::AluInstruction *ins)
{
   assert(ins->numSources == 2);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " * ";
   translateAluSource(state, ins->sources[1]);

   translateAluDestEnd(state, ins);
   return true;
}

static bool translateAluCNDE_INT(GenerateState &state, shadir::AluInstruction *ins)
{
   assert(ins->numSources == 3);
   translateAluDestStart(state, ins);

   state.out << "((";
   translateAluSource(state, ins->sources[0]);
   state.out << " == 0) ? ";
   translateAluSource(state, ins->sources[1]);
   state.out << " : ";
   translateAluSource(state, ins->sources[2]);
   state.out << ')';

   translateAluDestEnd(state, ins);
   return true;
}

static bool translateAluMULADD(GenerateState &state, shadir::AluInstruction *ins)
{
   assert(ins->numSources == 3);
   translateAluDestStart(state, ins);

   translateAluSource(state, ins->sources[0]);
   state.out << " * ";
   translateAluSource(state, ins->sources[1]);
   state.out << " + ";
   translateAluSource(state, ins->sources[2]);

   translateAluDestEnd(state, ins);
   return true;
}

static void initialise()
{
   static bool initialised = false;

   if (!initialised) {
      initialised = true;
      insTableCf[latte::cf::inst::LOOP_BREAK] = translateCfLOOP_BREAK;
      insTableCf[latte::cf::inst::LOOP_CONTINUE] = translateCfLOOP_CONTINUE;
      insTableAluOp2[latte::alu::op2::ADD] = translateAluADD;
      insTableAluOp2[latte::alu::op2::MOV] = translateAluMOV;
      insTableAluOp2[latte::alu::op2::FLT_TO_INT] = translateAluFLT_TO_INT;
      insTableAluOp2[latte::alu::op2::PRED_SETE_INT] = translateAluPRED_SETE_INT;
      insTableAluOp2[latte::alu::op2::PRED_SETNE_INT] = translateAluPRED_SETNE_INT;
      insTableAluOp2[latte::alu::op2::RECIP_IEEE] = translateAluRECIP_IEEE;
      insTableAluOp2[latte::alu::op2::SETGE_DX10] = translateAluSETGE_DX10;
      insTableAluOp2[latte::alu::op2::MUL_IEEE] = translateAluMUL_IEEE;
      insTableAluOp3[latte::alu::op3::CNDE_INT] = translateAluCNDE_INT;
      insTableAluOp3[latte::alu::op3::MULADD] = translateAluMULADD;
   }
}

static bool translateCfInstruction(GenerateState &state, shadir::CfInstruction *ins)
{
   auto itr = insTableCf.find(ins->id);

   if (itr == insTableCf.end()) {
      return false;
   } else {
      return (itr->second)(state, ins);
   }
}

static bool translateAluInstruction(GenerateState &state, shadir::AluInstruction *ins)
{
   if (ins->opType == shadir::AluInstruction::OP2) {
      auto itr = insTableAluOp2.find(ins->op2);

      if (itr == insTableAluOp2.end()) {
         return false;
      } else {
         return (itr->second)(state, ins);
      }
   } else {
      auto itr = insTableAluOp3.find(ins->op3);

      if (itr == insTableAluOp3.end()) {
         return false;
      } else {
         return (itr->second)(state, ins);
      }
   }
}

static bool translateTexInstruction(GenerateState &state, shadir::TexInstruction *ins)
{
   auto itr = insTableTex.find(ins->id);

   if (itr == insTableTex.end()) {
      return false;
   } else {
      return (itr->second)(state, ins);
   }
}

static bool translateInstruction(GenerateState &state, shadir::Instruction *ins)
{
   switch (ins->insType) {
   case shadir::Instruction::ControlFlow:
      return translateCfInstruction(state, reinterpret_cast<shadir::CfInstruction *>(ins));
   case shadir::Instruction::ALU:
      return translateAluInstruction(state, reinterpret_cast<shadir::AluInstruction *>(ins));
   case shadir::Instruction::TEX:
      return translateTexInstruction(state, reinterpret_cast<shadir::TexInstruction *>(ins));
   }

   return false;
}

static bool translateCodeBlock(GenerateState &state, shadir::CodeBlock *block)
{
   auto result = true;

   for (auto ins : block->code) {
      beginLine(state);
      result &= translateInstruction(state, ins);
      endLine(state);
   }

   return result;
}

static bool translateConditionalBlock(GenerateState &state, shadir::ConditionalBlock *block)
{
   auto result = true;

   beginLine(state);
   state.out << "if (";
   result &= translateInstruction(state, block->condition);
   state.out << ") {";
   endLine(state);

   increaseIndent(state);
   result &= translateBlocks(state, block->inner);
   decreaseIndent(state);

   if (block->innerElse.size()) {
      beginLine(state);
      state.out << "} else {";
      endLine(state);

      increaseIndent(state);
      result &= translateBlocks(state, block->inner);
      decreaseIndent(state);

      beginLine(state);
      state.out << "}";
      endLine(state);
   } else {
      beginLine(state);
      state.out << "}";
      endLine(state);
   }

   return result;
}

static bool translateLoopBlock(GenerateState &state, shadir::LoopBlock *block)
{
   auto result = true;
   beginLine(state);
   state.out << "while (true) {";
   endLine(state);

   increaseIndent(state);
   result &= translateBlocks(state, block->inner);
   decreaseIndent(state);

   beginLine(state);
   state.out << "}";
   endLine(state);
   return result;
}

static bool translateBlock(GenerateState &state, shadir::Block *block)
{
   switch (block->type) {
   case shadir::Block::CodeBlock:
      return translateCodeBlock(state, reinterpret_cast<shadir::CodeBlock *>(block));
   case shadir::Block::Conditional:
      return translateConditionalBlock(state, reinterpret_cast<shadir::ConditionalBlock *>(block));
   case shadir::Block::Loop:
      return translateLoopBlock(state, reinterpret_cast<shadir::LoopBlock *>(block));
   }

   return false;
}

static bool translateBlocks(GenerateState &state, std::vector<shadir::Block *> blocks)
{
   auto result = true;

   for (auto block : blocks) {
      result &= translateBlock(state, block);
   }

   return result;
}

bool generateHLSL(Shader &shader, std::string &hlsl)
{
   auto result = true;
   auto state = GenerateState {};

   initialise();
   result &= translateBlocks(state, shader.blocks);
   hlsl = state.out.c_str();

   return result;
}

};
