#include <algorithm>
#include "latte_shadir.h"
#include "latte_decoder.h"

namespace latte
{

static void
insertPush(Shader &shader, uint32_t cfPC)
{
   auto push = new shadir::CfInstruction {};
   push->cfPC = cfPC;
   push->id = SQ_CF_INST_PUSH;
   push->name = getInstructionName(push->id);
   shader.linear.push_back(push);
   shader.custom.emplace_back(push);
}

static void
insertPop(Shader &shader, uint32_t cfPC)
{
   auto pop = new shadir::CfInstruction {};
   pop->cfPC = cfPC;
   pop->id = SQ_CF_INST_PUSH;
   pop->name = getInstructionName(pop->id);
   pop->popCount = 1;
   shader.linear.push_back(pop);
   shader.custom.emplace_back(pop);
}

static void
insertBreak(Shader &shader, uint32_t cfPC)
{
   auto loopBreak = new shadir::CfInstruction {};
   loopBreak->cfPC = cfPC;
   loopBreak->id = SQ_CF_INST_LOOP_BREAK;
   loopBreak->name = getInstructionName(loopBreak->id);
   loopBreak->popCount = 1;
   shader.linear.push_back(loopBreak);
   shader.custom.emplace_back(loopBreak);
}

static void
insertContinue(Shader &shader, uint32_t cfPC)
{
   auto loopContinue = new shadir::CfInstruction {};
   loopContinue->cfPC = cfPC;
   loopContinue->id = SQ_CF_INST_LOOP_CONTINUE;
   loopContinue->name = getInstructionName(loopContinue->id);
   loopContinue->popCount = 1;
   shader.linear.push_back(loopContinue);
   shader.custom.emplace_back(loopContinue);
}

static void
insertElse(Shader &shader, uint32_t cfPC)
{
   auto elseIns = new shadir::CfInstruction {};
   elseIns->cfPC = cfPC;
   elseIns->id = SQ_CF_INST_ELSE;
   elseIns->name = getInstructionName(elseIns->id);
   elseIns->popCount = 1;
   shader.linear.push_back(elseIns);
   shader.custom.emplace_back(elseIns);
}


/**
 * Unpack instructions within clauses into a linear format.
 *
 * This will also explicitly insert the implied PUSH/POP/ELSE/BREAK/CONTINUE
 * for CF_ALU instructions to ease translation.
 *
 * Also tracks usages of registers for use in code generation.
 */
bool
linearify(Shader &shader)
{
   for (auto &ins : shader.code) {
      if (ins->type == shadir::Instruction::CF) {
         auto cfIns = reinterpret_cast<shadir::CfInstruction *>(ins.get());

         if (cfIns->clause.size()) {
            for (auto &child : cfIns->clause) {
               shader.linear.push_back(child.get());

               // Check for register usage
               if (child->type == shadir::Instruction::TEX) {
                  auto texIns = reinterpret_cast<shadir::TextureFetchInstruction *>(child.get());
                  shader.samplersUsed.emplace(texIns->samplerID);
                  shader.resourcesUsed.emplace(texIns->resourceID);
                  shader.gprsUsed.emplace(texIns->src.id);
                  shader.gprsUsed.emplace(texIns->dst.id);
               } else if (child->type == shadir::Instruction::VTX) {
                  auto vtxIns = reinterpret_cast<shadir::VertexFetchInstruction *>(child.get());
                  shader.gprsUsed.emplace(vtxIns->src.id);
                  shader.gprsUsed.emplace(vtxIns->dst.id);
               }
            }
         } else {
            shader.linear.push_back(cfIns);
         }
      } else if (ins->type == shadir::Instruction::CF_ALU) {
         auto cfAluIns = reinterpret_cast<shadir::CfAluInstruction *>(ins.get());
         auto pushedBefore = false;
         shadir::AluReductionInstruction *reduction = nullptr;

         switch (cfAluIns->id) {
         case SQ_CF_INST_ALU_BREAK:
         case SQ_CF_INST_ALU_CONTINUE:
            // Insert a push before ALU clause
            insertPush(shader, cfAluIns->cfPC);
            break;
         }

         for (auto &child : cfAluIns->clause) {
            auto aluIns = reinterpret_cast<shadir::AluInstruction *>(child.get());
            auto isPredSet = aluIns->flags & SQ_ALU_FLAG_PRED_SET;

            if (isPredSet) {
               switch (cfAluIns->id) {
               case SQ_CF_INST_ALU_PUSH_BEFORE:
                  // Insert a push only before the first PRED_SET_*
                  if (!pushedBefore) {
                     insertPush(shader, aluIns->cfPC);
                     pushedBefore = true;
                  }
                  break;
               case SQ_CF_INST_ALU_ELSE_AFTER:
                  // Insert a push before PRED_SET_*
                  insertPush(shader, aluIns->cfPC);
                  break;
               }
            }

            if (!reduction) {
               if (aluIns->op2 == SQ_OP2_INST_DOT4
                || aluIns->op2 == SQ_OP2_INST_DOT4_IEEE
                || aluIns->op2 == SQ_OP2_INST_CUBE
                || aluIns->op2 == SQ_OP2_INST_MAX4) {
                  // Combine this into reduction instruction
                  reduction = new shadir::AluReductionInstruction {};
                  reduction->op2 = aluIns->op2;
                  reduction->cfPC = aluIns->cfPC;
                  reduction->groupPC = aluIns->groupPC;
                  reduction->name = aluIns->name;
                  reduction->units.fill(nullptr);
                  shader.custom.emplace_back(reduction);
               }
            }

            if (reduction) {
               if (reduction->units[aluIns->unit]) {
                  throw std::logic_error("Reduction instruction unit collision");
               }

               aluIns->isReduction = true;
               reduction->units[aluIns->unit] = aluIns;

               if (std::all_of(reduction->units.begin(), reduction->units.end(), [](auto unit) { return !!unit; })) {
                  shader.linear.push_back(reduction);
                  reduction = nullptr;
               }
            } else {
               shader.linear.push_back(aluIns);
            }

            if (isPredSet) {
               switch (cfAluIns->id) {
               case SQ_CF_INST_ALU_BREAK:
                  // Insert a break after PRED_SET_*
                  insertBreak(shader, aluIns->cfPC);
                  break;
               case SQ_CF_INST_ALU_CONTINUE:
                  // Insert a continue after PRED_SET_*
                  insertContinue(shader, aluIns->cfPC);
                  break;
               case SQ_CF_INST_ALU_ELSE_AFTER:
                  // Insert an else after PRED_SET_*
                  insertElse(shader, aluIns->cfPC);
                  break;
               }
            }

            // Check for register usage
            for (auto i = 0u; i < aluIns->srcCount; ++i) {
               auto &src = aluIns->src[i];

               if (src.sel >= latte::SQ_ALU_REGISTER_0 && src.sel <= SQ_ALU_REGISTER_127) {
                  shader.gprsUsed.emplace(src.sel - latte::SQ_ALU_REGISTER_0);
               } else if (src.sel >= latte::SQ_ALU_SRC_CONST_FILE_0 && src.sel <= SQ_ALU_SRC_CONST_FILE_255) {
                  shader.uniformsUsed.emplace(src.sel - latte::SQ_ALU_SRC_CONST_FILE_0);
               } else if (src.sel == latte::SQ_ALU_SRC_PV) {
                  shader.pvUsed.emplace(aluIns->groupPC - 1);
               } else if (src.sel == latte::SQ_ALU_SRC_PS) {
                  shader.psUsed.emplace(aluIns->groupPC - 1);
               }
            }

            if (aluIns->writeMask) {
               shader.gprsUsed.emplace(aluIns->dst.sel);
            }
         }

         switch (cfAluIns->id) {
         case SQ_CF_INST_ALU_BREAK:
         case SQ_CF_INST_ALU_CONTINUE:
         case SQ_CF_INST_ALU_POP_AFTER:
            // Insert a pop after ALU clause
            insertPop(shader, cfAluIns->cfPC);
            break;
         case SQ_CF_INST_ALU_POP2_AFTER:
            // Insert 2 pops after ALU clause
            insertPop(shader, cfAluIns->cfPC);
            insertPop(shader, cfAluIns->cfPC);
            break;
         }
      } else {
         if (ins->type == shadir::Instruction::EXP) {
            auto expIns = reinterpret_cast<shadir::ExportInstruction *>(ins.get());
            shader.gprsUsed.emplace(expIns->rw.id);
         }

         shader.linear.push_back(ins.get());
      }
   }

   return true;
}

} // namespace latte
