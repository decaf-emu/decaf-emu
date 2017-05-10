#include "shader_compiler.h"
#include <common/align.h>

static const char *RootNode = "Program";
static const char *InstructionNode = "Instruction";
static const char *CfInstNode = "CfInst";
static const char *CfExpInstNode = "CfExpInst";
static const char *AluClauseNode = "AluClause";
static const char *EndOfProgramNode = "EndOfProgram";

static bool
compileCfInst(Shader &shader, peg::Ast &node)
{
   auto inst = latte::ControlFlowInst { 0 };

   inst.word1 = inst.word1
      .CF_INST_TYPE(latte::SQ_CF_INST_TYPE_NORMAL)
      .BARRIER(true);

   for (auto &child : node.nodes) {
      if (child->name == "InstCount") {
         auto cfPC = parseNumber(*child);

         if (cfPC != shader.cfInsts.size()) {
            throw incorrect_cf_pc_exception { *child, cfPC, shader.cfInsts.size() };
         }
      } else if (child->name == "CfOpcode") {
         auto &name = child->token;
         auto opcode = latte::getCfInstructionByName(name);

         if (opcode == latte::SQ_CF_INST_INVALID) {
            throw invalid_cf_inst_exception { *child };
         }

         inst.word1 = inst.word1.CF_INST(opcode);
      } else if (child->name == "CfInstProperties") {
         for (auto &prop : child->nodes) {
            if (prop->name == "NO_BARRIER") {
               inst.word1 = inst.word1.BARRIER(false);
            } else if (prop->name == "WHOLE_QUAD_MODE") {
               inst.word1 = inst.word1.WHOLE_QUAD_MODE(true);
            } else if (prop->name == "VALID_PIX") {
               inst.word1 = inst.word1.VALID_PIXEL_MODE(true);
            } else {
               throw invalid_cf_property_exception { *prop };
            }
         }
      } else {
         throw unhandled_node_exception { *child };
      }
   }

   shader.cfInsts.push_back(inst);
   return true;
}

static bool
compileInstruction(Shader &shader, peg::Ast &node)
{
   if (node.name == CfInstNode) {
      return compileCfInst(shader, node);
   } else if (node.name == CfExpInstNode) {
      return compileExpInst(shader, node);
   } else if (node.name == AluClauseNode) {
      return compileAluClause(shader, node);
   } else {
      throw unhandled_node_exception { node };
   }

   return false;
}

static bool
compileClauses(Shader &shader)
{
   auto aluClauseData = std::vector<uint32_t> {};
   auto baseAddress = align_up(shader.cfInsts.size(), 256 / 8);

   for (auto &clause : shader.aluClauses) {
      auto &cfInst = shader.cfInsts[clause.cfPC];
      auto offset = aluClauseData.size() / 2;

      for (auto &group : clause.groups) {
         for (auto inst : group.insts) {
            aluClauseData.push_back(inst.word0.value);
            aluClauseData.push_back(inst.word1.value);
         }

         for (auto literal : group.literals) {
            aluClauseData.push_back(literal.hexValue);
         }

         if (group.literals.size() % 2) {
            // Must pad to 64 bit
            aluClauseData.push_back(0);
         }
      }

      auto addr = baseAddress + offset;
      auto count = ((aluClauseData.size() - offset) / 2) - 1; // Count is -1

      if (clause.addrNode) {
         if (cfInst.alu.word0.ADDR() != addr) {
            throw incorrect_clause_addr_exception { *clause.countNode, cfInst.alu.word0.ADDR(), addr };
         }
      } else {
         cfInst.alu.word0 = cfInst.alu.word0
            .ADDR(static_cast<uint32_t>(addr));
      }

      if (clause.countNode) {
         if (cfInst.alu.word1.COUNT() != count) {
            throw incorrect_clause_count_exception { *clause.countNode, cfInst.alu.word1.COUNT(), count };
         }
      } else {
         cfInst.alu.word1 = cfInst.alu.word1
            .COUNT(static_cast<uint32_t>(count));
      }
   }

   // TODO: Same for TEX, VTX clauses

   shader.aluClauseBaseAddress = static_cast<uint32_t>(baseAddress);
   shader.aluClauseData = std::move(aluClauseData);
   return true;
}

static bool
compileEndOfProgram(Shader &shader)
{
   if (shader.cfInsts.size()) {
      auto &last = shader.cfInsts.back();
      if (last.word1.CF_INST_TYPE() == latte::SQ_CF_INST_TYPE_NORMAL
       || last.word1.CF_INST_TYPE() == latte::SQ_CF_INST_TYPE_EXPORT) {
         last.word1 = last.word1
            .END_OF_PROGRAM(true);
         return true;
      }
   }

   auto inst = latte::ControlFlowInst { 0 };
   inst.word1 = inst.word1
      .CF_INST(latte::SQ_CF_INST_NOP)
      .CF_INST_TYPE(latte::SQ_CF_INST_TYPE_NORMAL)
      .END_OF_PROGRAM(true);

   return true;
}

bool
compileAST(Shader &shader, std::shared_ptr<peg::Ast> ast)
{
   auto foundEOP = false;

   if (ast->name != RootNode) {
      return false;
   }

   for (auto &node : ast->nodes) {
      if (node->name == InstructionNode) {
         assert(node->nodes.size() == 1);
         compileInstruction(shader, *node->nodes[0]);
      } else if (node->name == EndOfProgramNode) {
         compileEndOfProgram(shader);
         foundEOP = true;
      } else if (node->name == "Comment") {
         if (node->token.size()) {
            shader.comments.push_back(node->token);
         }
      } else {
         throw unhandled_node_exception { *node };
      }
   }

   // If the user did not add a END_OF_PROGRAM, we should do it automatically
   if (!foundEOP) {
      compileEndOfProgram(shader);
   }

   if (!compileClauses(shader)) {
      return false;
   }

   return true;
}
