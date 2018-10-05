#include "shader_compiler.h"

#include <common/align.h>
#include <common/bit_cast.h>
#include <fmt/format.h>

static const size_t
AluClauseAlign = 256;

static const size_t
TexClauseAlign = 128;

static void
compileCfInst(Shader &shader, peg::Ast &node)
{
   auto inst = latte::ControlFlowInst { };

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
}

static void
compileInstruction(Shader &shader, peg::Ast &node)
{
   if (node.name == "CfInst") {
      compileCfInst(shader, node);
   } else if (node.name == "CfExpInst") {
      compileExpInst(shader, node);
   } else if (node.name == "AluClause") {
      compileAluClause(shader, node);
   } else if (node.name == "TexClause") {
      compileTexClause(shader, node);
   } else {
      throw unhandled_node_exception { node };
   }
}

static void
compileClauses(Shader &shader)
{
   auto aluClauseData = std::vector<uint32_t> {};
   auto texClauseData = std::vector<uint32_t> {};
   auto aluClauseBaseAddress = align_up(shader.cfInsts.size(), AluClauseAlign / 8);

   for (auto &clause : shader.aluClauses) {
      auto &cfInst = shader.cfInsts[clause.cfPC];
      auto offset = aluClauseData.size();

      for (auto &group : clause.groups) {
         for (auto inst : group.insts) {
            aluClauseData.push_back(inst.word0.value);
            aluClauseData.push_back(inst.word1.value);
         }

         for (auto literal : group.literals) {
            if (literal.flags & LiteralValue::ReadHex) {
               aluClauseData.push_back(literal.hexValue);
            } else {
               aluClauseData.push_back(bit_cast<uint32_t>(literal.floatValue));
            }
         }

         if (group.literals.size() % 2) {
            // Must pad to 64 bit
            aluClauseData.push_back(0);
         }
      }

      auto addr = aluClauseBaseAddress + (offset / 2);
      auto count = (aluClauseData.size() - offset) / 2;

      if (clause.addrNode) {
         if (cfInst.alu.word0.ADDR() != addr) {
            throw incorrect_clause_addr_exception { *clause.countNode, cfInst.alu.word0.ADDR(), addr };
         }
      } else {
         cfInst.alu.word0 = cfInst.alu.word0
            .ADDR(static_cast<uint32_t>(addr));
      }

      if (clause.countNode) {
         auto parsedCount = cfInst.alu.word1.COUNT() + 1;

         if (parsedCount != count) {
            throw incorrect_clause_count_exception { *clause.countNode, parsedCount, count };
         }
      } else {
         cfInst.alu.word1 = cfInst.alu.word1
            .COUNT(static_cast<uint32_t>(count - 1));
      }
   }

   if (aluClauseData.size()) {
      shader.aluClauseBaseAddress = static_cast<uint32_t>(aluClauseBaseAddress);
      shader.aluClauseData = std::move(aluClauseData);
   } else {
      shader.aluClauseBaseAddress = 0;
   }

   auto texClauseBaseAddress = align_up(aluClauseBaseAddress + shader.aluClauseData.size() / 2, TexClauseAlign / 8);

   for (auto &clause : shader.texClauses) {
      auto &cfInst = shader.cfInsts[clause.cfPC];
      auto offset = texClauseData.size();

      for (auto &inst : clause.insts) {
         texClauseData.push_back(inst.word0.value);
         texClauseData.push_back(inst.word1.value);
         texClauseData.push_back(inst.word2.value);
         texClauseData.push_back(inst.padding);
      }

      auto addr = texClauseBaseAddress + (offset / 2);
      auto count = (texClauseData.size() - offset) / 4;

      if (clause.addrNode) {
         if (cfInst.word0.ADDR() != addr) {
            throw incorrect_clause_addr_exception { *clause.countNode, cfInst.word0.ADDR(), addr };
         }
      } else {
         cfInst.word0 = cfInst.word0
            .ADDR(static_cast<uint32_t>(addr));
      }

      if (clause.countNode) {
         auto parsedCount = (cfInst.word1.COUNT() | (cfInst.word1.COUNT_3() << 3)) + 1;

         if (parsedCount != count) {
            throw incorrect_clause_count_exception { *clause.countNode, parsedCount, count };
         }
      } else {
         cfInst.word1 = cfInst.word1
            .COUNT(static_cast<uint32_t>((count - 1) & 0b111))
            .COUNT_3(static_cast<uint32_t>((count - 1) >> 3));
      }
   }

   if (texClauseData.size()) {
      shader.texClauseBaseAddress = static_cast<uint32_t>(texClauseBaseAddress);
      shader.texClauseData = std::move(texClauseData);
   } else {
      shader.texClauseBaseAddress = 0;
   }

   // TODO: Same for VTX clauses
}

static void
compileEndOfProgram(Shader &shader)
{
   if (shader.cfInsts.size()) {
      auto &last = shader.cfInsts.back();
      if (last.word1.CF_INST_TYPE() == latte::SQ_CF_INST_TYPE_NORMAL
       || last.word1.CF_INST_TYPE() == latte::SQ_CF_INST_TYPE_EXPORT) {
         last.word1 = last.word1
            .END_OF_PROGRAM(true);
         return;
      }
   }

   auto inst = latte::ControlFlowInst { };
   inst.word1 = inst.word1
      .CF_INST(latte::SQ_CF_INST_NOP)
      .CF_INST_TYPE(latte::SQ_CF_INST_TYPE_NORMAL)
      .END_OF_PROGRAM(true);
}

void
compileAST(Shader &shader,
           std::shared_ptr<peg::Ast> ast)
{
   auto foundEOP = false;

   if (ast->name != "Program") {
      throw node_parse_exception { *ast, fmt::format("Expected root node to be Program, not {}", ast->name) };
   }

   for (auto &node : ast->nodes) {
      if (node->name == "Instruction") {
         assert(node->nodes.size() == 1);
         compileInstruction(shader, *node->nodes[0]);
      } else if (node->name == "EndOfProgram") {
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

   compileClauses(shader);
}
