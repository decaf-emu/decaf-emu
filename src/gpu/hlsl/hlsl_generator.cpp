#define NOMINMAX
#include <spdlog/spdlog.h>
#include "gpu/latte.h"
#include "gpu/hlsl/hlsl_generator.h"

static const auto indentSize = 3u;

namespace hlsl
{

static bool translateBlocks(GenerateState &state, std::vector<latte::shadir::Block *> blocks);

static std::map<latte::cf::inst, TranslateFuncCF> sGeneratorTableCf;
static std::map<latte::alu::op2, TranslateFuncALU> sGeneratorTableAluOp2;
static std::map<latte::alu::op3, TranslateFuncALU> sGeneratorTableAluOp3;
static std::map<latte::alu::op2, TranslateFuncALUReduction> sGeneratorTableAluOp2Reduction;
static std::map<latte::tex::inst, TranslateFuncTEX> sGeneratorTableTex;

void intialise()
{
   static bool initialised = false;

   if (!initialised) {
      registerCf();
      registerAluOP2();
      registerAluOP3();
      registerAluReduction();
      registerTex();
   }
}

void registerGenerator(latte::cf::inst ins, TranslateFuncCF func)
{
   sGeneratorTableCf[ins] = func;
}

void registerGenerator(latte::alu::op2 ins, TranslateFuncALU func)
{
   sGeneratorTableAluOp2[ins] = func;
}

void registerGenerator(latte::alu::op3 ins, TranslateFuncALU func)
{
   sGeneratorTableAluOp3[ins] = func;
}

void registerGenerator(latte::alu::op2 ins, TranslateFuncALUReduction func)
{
   sGeneratorTableAluOp2Reduction[ins] = func;
}

void registerGenerator(latte::tex::inst ins, TranslateFuncTEX func)
{
   sGeneratorTableTex[ins] = func;
}

void beginLine(GenerateState &state)
{
   state.out << state.indent.c_str();
}

void endLine(GenerateState &state)
{
   state.out << '\n';
}

void increaseIndent(GenerateState &state)
{
   state.indent.append(indentSize, ' ');
}

void decreaseIndent(GenerateState &state)
{
   if (state.indent.size() >= indentSize) {
      state.indent.resize(state.indent.size() - indentSize);
   }
}

static bool translateInstruction(GenerateState &state, latte::shadir::Instruction *ins)
{
   switch (ins->insType) {
   case latte::shadir::Instruction::ControlFlow:
      {
         auto ins2 = reinterpret_cast<latte::shadir::CfInstruction *>(ins);
         auto itr = sGeneratorTableCf.find(ins2->id);

         if (itr == sGeneratorTableCf.end()) {
            return false;
         } else {
            return (*itr->second)(state, ins2);
         }
      }
      break;
   case latte::shadir::Instruction::TEX:
      {
         auto ins2 = reinterpret_cast<latte::shadir::TexInstruction *>(ins);
         auto itr = sGeneratorTableTex.find(ins2->id);

         if (itr == sGeneratorTableTex.end()) {
            return false;
         } else {
            return (*itr->second)(state, ins2);
         }
      }
      break;
   case latte::shadir::Instruction::AluReduction:
      {
         auto ins2 = reinterpret_cast<latte::shadir::AluReductionInstruction *>(ins);
         auto itr = sGeneratorTableAluOp2Reduction.find(ins2->op2);

         if (itr == sGeneratorTableAluOp2Reduction.end()) {
            return false;
         } else {
            return (*itr->second)(state, ins2);
         }
      }
      break;
   case latte::shadir::Instruction::ALU:
      {
         auto ins2 = reinterpret_cast<latte::shadir::AluInstruction *>(ins);

         if (ins2->opType == latte::shadir::AluInstruction::OP2) {
            auto itr = sGeneratorTableAluOp2.find(ins2->op2);

            if (itr == sGeneratorTableAluOp2.end()) {
               return false;
            } else {
               return (*itr->second)(state, ins2);
            }
         } else {
            auto itr = sGeneratorTableAluOp3.find(ins2->op3);

            if (itr == sGeneratorTableAluOp3.end()) {
               return false;
            } else {
               return (*itr->second)(state, ins2);
            }
         }
      }
      break;
   }

   return false;
}

static bool translateCodeBlock(GenerateState &state, latte::shadir::CodeBlock *block)
{
   auto result = true;

   for (auto ins : block->code) {
      beginLine(state);
      result &= translateInstruction(state, ins);
      endLine(state);
   }

   return result;
}

static bool translateConditionalBlock(GenerateState &state, latte::shadir::ConditionalBlock *block)
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

static bool translateLoopBlock(GenerateState &state, latte::shadir::LoopBlock *block)
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

static bool translateBlock(GenerateState &state, latte::shadir::Block *block)
{
   switch (block->type) {
   case latte::shadir::Block::CodeBlock:
      return translateCodeBlock(state, reinterpret_cast<latte::shadir::CodeBlock *>(block));
   case latte::shadir::Block::Conditional:
      return translateConditionalBlock(state, reinterpret_cast<latte::shadir::ConditionalBlock *>(block));
   case latte::shadir::Block::Loop:
      return translateLoopBlock(state, reinterpret_cast<latte::shadir::LoopBlock *>(block));
   }

   return false;
}

static bool translateBlocks(GenerateState &state, std::vector<latte::shadir::Block *> blocks)
{
   auto result = true;

   for (auto block : blocks) {
      result &= translateBlock(state, block);
   }

   return result;
}

} // namespace hlsl

namespace latte
{

bool generateHLSL(Shader &shader, std::string &hlsl)
{
   auto result = true;
   auto state = GenerateState {};

   hlsl::intialise();
   result &= hlsl::translateBlocks(state, shader.blocks);
   hlsl = state.out.c_str();

   return result;
}

} // namespace latte
