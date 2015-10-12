#include <array>
#include <cassert>
#include <iostream>
#include <set>
#include "latte_shadir.h"
#include "latte.h"

namespace latte
{

struct AnalyseState
{
   struct Register
   {
      std::array<int32_t, 4> read = { -1, -1, -1, -1 };
      std::array<int32_t, 4> write = { -1, -1, -1, -1 };
   };

   int32_t addr = -1;
   uint32_t cfPC;
   uint32_t groupPC;
   std::array<Register, 512> gpr;
   Shader *shader;
};

static bool analyseBlocks(AnalyseState &state, latte::shadir::BlockList &blocks);
static bool analyseBlock(AnalyseState &state, latte::shadir::Block *block);
static bool analyseLoopBlock(AnalyseState &state, latte::shadir::LoopBlock *block);
static bool analyseConditionalBlock(AnalyseState &state, latte::shadir::ConditionalBlock *block);
static bool analyseCodeBlock(AnalyseState &state, latte::shadir::CodeBlock *block);
static bool analyseInstruction(AnalyseState &state, latte::shadir::Instruction *instr);
static bool analyseInstruction(AnalyseState &state, latte::shadir::CfInstruction *ins);
static bool analyseInstruction(AnalyseState &state, latte::shadir::TexInstruction *ins);
static bool analyseInstruction(AnalyseState &state, latte::shadir::AluReductionInstruction *ins);
static bool analyseInstruction(AnalyseState &state, latte::shadir::AluInstruction *ins);
static bool analyseInstruction(AnalyseState &state, latte::shadir::ExportInstruction *ins);

static bool
isValidReadSel(latte::alu::Select::Select sel)
{
   switch (sel) {
   case latte::alu::Select::X:
   case latte::alu::Select::Y:
   case latte::alu::Select::Z:
   case latte::alu::Select::W:
      return true;
   default:
      return false;
   }
}

static bool
isValidChannel(latte::alu::Channel::Channel sel)
{
   switch (sel) {
   case latte::alu::Channel::X:
   case latte::alu::Channel::Y:
   case latte::alu::Channel::Z:
   case latte::alu::Channel::W:
      return true;
   default:
      return false;
   }
}

static bool
updateRegisterRead(AnalyseState &state, uint32_t id, uint32_t channel)
{
   if (state.addr == -1) {
      return false;
   }

   if (id >= state.gpr.size()) {
      return false;
   }

   state.shader->gprsUsed.emplace(id);

   if (channel >= state.gpr[id].read.size()) {
      return false;
   }

   if (state.gpr[id].read[channel] != -1) {
      return false;
   }

   state.gpr[id].read[channel] = state.addr;
   return true;
}

static bool
updateRegisterWrite(AnalyseState &state, uint32_t id, uint32_t channel)
{
   if (state.addr == -1) {
      return false;
   }

   if (id >= state.gpr.size()) {
      return false;
   }

   if (channel >= state.gpr[id].write.size()) {
      return false;
   }

   state.shader->gprsUsed.emplace(id);

   if (state.gpr[id].write[channel] != -1) {
      return false;
   }

   state.gpr[id].write[channel] = state.addr;
   return true;
}

static void
setRegisterReadSel(AnalyseState &state, latte::shadir::SelRegister &src)
{
   updateRegisterRead(state, src.id, src.selX);
   updateRegisterRead(state, src.id, src.selY);
   updateRegisterRead(state, src.id, src.selZ);
   updateRegisterRead(state, src.id, src.selW);
}

static void
setRegisterWriteSel(AnalyseState &state, latte::shadir::SelRegister &dst)
{
   updateRegisterWrite(state, dst.id, dst.selX);
   updateRegisterWrite(state, dst.id, dst.selY);
   updateRegisterWrite(state, dst.id, dst.selZ);
   updateRegisterWrite(state, dst.id, dst.selW);
}

static bool
analyseInstruction(AnalyseState &state, latte::shadir::CfInstruction *ins)
{
   return true;
}

static bool
analyseInstruction(AnalyseState &state, latte::shadir::TexInstruction *ins)
{
   state.shader->samplersUsed.emplace(ins->samplerID);
   state.shader->resourcesUsed.emplace(ins->resourceID);
   setRegisterReadSel(state, ins->src);
   setRegisterWriteSel(state, ins->dst);
   return true;
}

static void
analyseAluSource(AnalyseState &state, latte::shadir::AluSource &src)
{
   /*
   TODO:
   KcacheBank0,
   KcacheBank1,
   PreviousVector,
   PreviousScalar,
   ConstantFile,
   ConstantFloat,
   ConstantDouble,
   ConstantInt,
   */
   switch (src.type) {
   case latte::shadir::AluSource::Register:
      assert(src.rel == false);
      updateRegisterRead(state, src.id, src.chan);
      break;
   case latte::shadir::AluSource::PreviousVector:
      if (state.groupPC != -1) {
         state.shader->pvUsed.emplace(state.groupPC - 1);
      }
      break;
   case latte::shadir::AluSource::PreviousScalar:
      if (state.groupPC != -1) {
         state.shader->psUsed.emplace(state.groupPC - 1);
      }
      break;
   }
}

static void
analyseAluDest(AnalyseState &state, latte::shadir::AluDest &dst)
{
   updateRegisterWrite(state, dst.id, dst.chan);
}

static bool
analyseInstruction(AnalyseState &state, latte::shadir::AluInstruction *ins)
{
   for (auto i = 0u; i < ins->numSources; ++i) {
      analyseAluSource(state, ins->sources[i]);
   }

   analyseAluDest(state, ins->dest);
   return true;
}

static bool
analyseInstruction(AnalyseState &state, latte::shadir::AluReductionInstruction *ins)
{
   bool result = true;

   for (auto i = 0u; i < 4; ++i) {
      if (ins->units[i]) {
         result &= analyseInstruction(state, ins->units[i].get());
      }
   }

   return result;
}

static bool
analyseInstruction(AnalyseState &state, latte::shadir::ExportInstruction *ins)
{
   setRegisterReadSel(state, ins->src);
   return true;
}

static bool
analyseInstruction(AnalyseState &state, latte::shadir::Instruction *ins)
{
   state.cfPC = ins->cfPC;
   state.groupPC = ins->groupPC;

   if (ins->cfPC != -1) {
      state.addr = ins->cfPC << 16;

      if (ins->groupPC != -1) {
         state.addr |= ins->groupPC;
      }
   } else {
      state.addr = -1;
   }

   switch (ins->insType) {
   case latte::shadir::Instruction::ControlFlow:
      return analyseInstruction(state, reinterpret_cast<latte::shadir::CfInstruction *>(ins));
   case latte::shadir::Instruction::TEX:
      return analyseInstruction(state, reinterpret_cast<latte::shadir::TexInstruction *>(ins));
   case latte::shadir::Instruction::AluReduction:
      return analyseInstruction(state, reinterpret_cast<latte::shadir::AluReductionInstruction *>(ins));
   case latte::shadir::Instruction::ALU:
      return analyseInstruction(state, reinterpret_cast<latte::shadir::AluInstruction *>(ins));
   case latte::shadir::Instruction::Export:
      return analyseInstruction(state, reinterpret_cast<latte::shadir::ExportInstruction *>(ins));
   }

   return false;
}


static bool
analyseCodeBlock(AnalyseState &state, latte::shadir::CodeBlock *block)
{
   auto result = true;

   for (auto ins : block->code) {
      result &= analyseInstruction(state, ins);
   }

   return result;
}


static bool
analyseConditionalBlock(AnalyseState &state, latte::shadir::ConditionalBlock *block)
{
   auto result = true;

   result &= analyseInstruction(state, block->condition);
   result &= analyseBlocks(state, block->inner);

   if (block->innerElse.size()) {
      result &= analyseBlocks(state, block->innerElse);
   }

   return result;
}


static bool
analyseLoopBlock(AnalyseState &state, latte::shadir::LoopBlock *block)
{
   auto result = true;
   result &= analyseBlocks(state, block->inner);
   return result;
}


static bool
analyseBlock(AnalyseState &state, latte::shadir::Block *block)
{
   switch (block->type) {
   case latte::shadir::Block::CodeBlock:
      return analyseCodeBlock(state, reinterpret_cast<latte::shadir::CodeBlock *>(block));
   case latte::shadir::Block::Conditional:
      return analyseConditionalBlock(state, reinterpret_cast<latte::shadir::ConditionalBlock *>(block));
   case latte::shadir::Block::Loop:
      return analyseLoopBlock(state, reinterpret_cast<latte::shadir::LoopBlock *>(block));
   }

   return false;
}


static bool
analyseBlocks(AnalyseState &state, latte::shadir::BlockList &blocks)
{
   auto result = true;

   for (auto &block : blocks) {
      result &= analyseBlock(state, block.get());
   }

   return result;
}


bool
analyse(Shader &shader)
{
   AnalyseState state;
   state.shader = &shader;
   analyseBlocks(state, shader.blocks);

   if (0) {
      for (auto id = 0u; id < state.gpr.size(); ++id) {
         auto &gpr = state.gpr[id];
         bool readBeforeWrite = false;

         for (auto chan = 0u; chan < 1 && !readBeforeWrite; ++chan) {
            if (gpr.write[chan] == -1) {
               if (gpr.read[chan] != -1) {
                  // Read without a write
                  readBeforeWrite = true;
               }
            } else if (gpr.read[chan] < gpr.write[chan]) {
               // Read before write
               readBeforeWrite = true;
            }
         }

         if (readBeforeWrite) {
            std::cout << "read before write " << id << std::endl;
         }
      }
   }

   return true;
}

} // namespace latte
