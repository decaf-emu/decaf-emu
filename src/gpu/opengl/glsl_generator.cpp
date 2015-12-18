#include <spdlog/spdlog.h>
#include "glsl_generator.h"
#include "modules/gx2/gx2_shaders.h"

static const auto indentSize = 3u;

namespace gpu
{

namespace opengl
{

namespace glsl
{

static bool translateBlocks(GenerateState &state, latte::shadir::BlockList &blocks);

static std::map<latte::SQ_CF_INST, TranslateFuncCF> sGeneratorTableCf;
static std::map<latte::SQ_OP2_INST, TranslateFuncALU> sGeneratorTableAluOp2;
static std::map<latte::SQ_OP3_INST, TranslateFuncALU> sGeneratorTableAluOp3;
static std::map<latte::SQ_OP2_INST, TranslateFuncALUReduction> sGeneratorTableAluOp2Reduction;
static std::map<latte::SQ_TEX_INST, TranslateFuncTEX> sGeneratorTableTex;
static std::map<latte::SQ_TEX_INST, TranslateFuncVTX> sGeneratorTableVtx;
static std::map<latte::SQ_CF_EXP_INST, TranslateFuncEXP> sGeneratorTableExport;


void
intialise()
{
   static bool initialised = false;

   if (!initialised) {
      registerCf();
      registerAluOP2();
      registerAluOP3();
      registerAluReduction();
      registerTex();
      registerVtx();
      registerExp();
   }
}


void
registerGenerator(latte::SQ_CF_INST ins, TranslateFuncCF func)
{
   sGeneratorTableCf[ins] = func;
}


void
registerGenerator(latte::SQ_OP2_INST ins, TranslateFuncALU func)
{
   sGeneratorTableAluOp2[ins] = func;
}


void
registerGenerator(latte::SQ_OP3_INST ins, TranslateFuncALU func)
{
   sGeneratorTableAluOp3[ins] = func;
}


void
registerGenerator(latte::SQ_OP2_INST ins, TranslateFuncALUReduction func)
{
   sGeneratorTableAluOp2Reduction[ins] = func;
}


void
registerGenerator(latte::SQ_TEX_INST ins, TranslateFuncTEX func)
{
   sGeneratorTableTex[ins] = func;
}


void
registerGenerator(latte::SQ_TEX_INST ins, TranslateFuncVTX func)
{
   sGeneratorTableVtx[ins] = func;
}


void
registerGenerator(latte::SQ_CF_EXP_INST ins, TranslateFuncEXP func)
{
   sGeneratorTableExport[ins] = func;
}


void
beginLine(GenerateState &state)
{
   state.out << state.indent.c_str();
}


void
endLine(GenerateState &state)
{
   state.out << '\n';
}


void
increaseIndent(GenerateState &state)
{
   state.indent.append(indentSize, ' ');
}


void
decreaseIndent(GenerateState &state)
{
   if (state.indent.size() >= indentSize) {
      state.indent.resize(state.indent.size() - indentSize);
   }
}


static bool
translateInstruction(GenerateState &state, latte::shadir::Instruction *ins)
{
   switch (ins->type) {
   case latte::shadir::Instruction::CF:
   {
      auto ins2 = reinterpret_cast<latte::shadir::CfInstruction *>(ins);
      auto itr = sGeneratorTableCf.find(ins2->id);

      if (itr == sGeneratorTableCf.end()) {
         state.out << "// Unimplemented " << ins->name;
         return false;
      } else {
         return (*itr->second)(state, ins2);
      }
      break;
   }
   case latte::shadir::Instruction::ALU:
   {
      auto ins2 = reinterpret_cast<latte::shadir::AluInstruction *>(ins);

      if (ins2->encoding == latte::SQ_ALU_OP2) {
         auto itr = sGeneratorTableAluOp2.find(ins2->op2);

         if (itr == sGeneratorTableAluOp2.end()) {
            state.out << "// Unimplemented " << ins->name;
            return false;
         } else {
            return (*itr->second)(state, ins2);
         }
      } else {
         auto itr = sGeneratorTableAluOp3.find(ins2->op3);

         if (itr == sGeneratorTableAluOp3.end()) {
            state.out << "// Unimplemented " << ins->name;
            return false;
         } else {
            return (*itr->second)(state, ins2);
         }
      }

      break;
   }
   case latte::shadir::Instruction::TEX:
   {
      auto ins2 = reinterpret_cast<latte::shadir::TextureFetchInstruction *>(ins);
      auto itr = sGeneratorTableTex.find(ins2->id);

      if (itr == sGeneratorTableTex.end()) {
         state.out << "// Unimplemented " << ins->name;
         return false;
      } else {
         return (*itr->second)(state, ins2);
      }

      break;
   }
   case latte::shadir::Instruction::VTX:
   {
      auto ins2 = reinterpret_cast<latte::shadir::VertexFetchInstruction *>(ins);
      auto itr = sGeneratorTableVtx.find(ins2->id);

      if (itr == sGeneratorTableVtx.end()) {
         state.out << "// Unimplemented " << ins->name;
         return false;
      } else {
         return (*itr->second)(state, ins2);
      }

      break;
   }
   case latte::shadir::Instruction::EXP:
   {
      auto ins2 = reinterpret_cast<latte::shadir::ExportInstruction *>(ins);
      auto itr = sGeneratorTableExport.find(ins2->id);

      if (itr == sGeneratorTableExport.end()) {
         state.out << "// Unimplemented " << ins->name;
         return false;
      } else {
         return (*itr->second)(state, ins2);
      }

      break;
   }
   case latte::shadir::Instruction::ALU_REDUCTION:
   {
      auto ins2 = reinterpret_cast<latte::shadir::AluReductionInstruction *>(ins);
      auto itr = sGeneratorTableAluOp2Reduction.find(ins2->op2);

      if (itr == sGeneratorTableAluOp2Reduction.end()) {
         state.out << "// Unimplemented " << ins->name;
         return false;
      } else {
         return (*itr->second)(state, ins2);
      }
      break;
   }
   case latte::shadir::Instruction::CF_ALU:
      throw std::logic_error("There should be no CF_ALU instructions in the shader blockify'd code");
      break;
   case latte::shadir::Instruction::Invalid:
      throw std::logic_error("Invalid instruction type encountered");
      break;
   }

   return false;
}


static bool
translateCodeBlock(GenerateState &state, latte::shadir::CodeBlock *block)
{
   auto result = true;

   for (auto &ins : block->code) {
      if (ins->groupPC != -1 && state.groupPC < ins->groupPC) {
         // If the last group both read and writes PV then we use the temp PVo / PSo to prevent overwrites of read
         if (state.shader->pvUsed.find(ins->groupPC - 1) != state.shader->pvUsed.end()) {
            if (state.shader->pvUsed.find(ins->groupPC - 2) != state.shader->pvUsed.end()) {
               beginLine(state);
               state.out << "PV = PVo;";
               endLine(state);
            }
         }

         if (state.shader->psUsed.find(ins->groupPC - 1) != state.shader->psUsed.end()) {
            if (state.shader->pvUsed.find(ins->groupPC - 2) != state.shader->pvUsed.end()) {
               beginLine(state);
               state.out << "PS = PSo;";
               endLine(state);
            }
         }

         // Begin new group
         beginLine(state);
         state.out << "// groupPC = " << ins->groupPC;
         endLine(state);
      }

      state.cfPC = ins->cfPC;
      state.groupPC = ins->groupPC;

      beginLine(state);
      result &= translateInstruction(state, ins);
      state.out << ";";
      endLine(state);
   }

   return result;
}


static bool
translateConditionalBlock(GenerateState &state, latte::shadir::ConditionalBlock *block)
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
      result &= translateBlocks(state, block->innerElse);
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


static bool
translateLoopBlock(GenerateState &state, latte::shadir::LoopBlock *block)
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


static bool
translateBlock(GenerateState &state, latte::shadir::Block *block)
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


static bool
translateBlocks(GenerateState &state, latte::shadir::BlockList &blocks)
{
   auto result = true;

   for (auto &block : blocks) {
      result &= translateBlock(state, block.get());
   }

   return result;
}

bool
generateBody(latte::Shader &shader, std::string &body)
{
   GenerateState state;
   state.shader = &shader;

   intialise();
   auto result = translateBlocks(state, shader.blocks);
   body = state.out.c_str();

   return result;
}

} // namespace glsl

} // namespace opengl

} // namespace gpu
