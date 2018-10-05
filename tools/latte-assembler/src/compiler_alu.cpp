#include "shader_compiler.h"
#include <fmt/format.h>

static std::string
decodeOpcodeAlias(const std::string &op)
{
   if (op == "SQRT_e") {
      return "SQRT_IEEE";
   } else if (op == "EXP_e") {
      return "EXP_IEEE";
   } else if (op == "LOG_e") {
      return "LOG_IEEE";
   } else if (op == "RSQ_e") {
      return "RECIPSQRT_IEEE";
   } else if (op == "RCP_e") {
      return "RECIP_IEEE";
   } else if (op == "LOG_sat") {
      return "LOG_CLAMPED";
   } else if (op == "MUL_e") {
      return "MUL_IEEE";
   } else if (op == "DOT4_e") {
      return "DOT4_IEEE";
   } else if (op == "MULADD_e") {
      return "MULADD_IEEE";
   } else {
      return op;
   }
}

static void
compileAluInst(Shader &shader, AluGroup &group, peg::Ast &node, unsigned numSrcs)
{
   auto inst = latte::AluInst {};
   auto srcIndex = 0;
   auto aluUnit = latte::SQ_CHAN {};
   std::memset(&inst, 0, sizeof(latte::AluInst));

   inst.op2 = inst.op2
      .WRITE_MASK(true);

   for (auto &child : node.nodes) {
      if (child->name == "AluUnit") {
         aluUnit = parseChan(*child);

         if (aluUnit != latte::SQ_CHAN::T) {
            inst.word1 = inst.word1
               .DST_CHAN(aluUnit);
         }
      } else if (child->name == "AluOpcode0" || child->name == "AluOpcode1" || child->name == "AluOpcode2") {
         const auto name = decodeOpcodeAlias(child->token);
         const auto opcode = latte::getAluOp2InstructionByName(name);

         if (opcode == latte::SQ_OP2_INST_INVALID) {
            throw invalid_alu_op2_inst_exception { *child };
         }

         inst.word1 = inst.word1
            .ENCODING(latte::SQ_ALU_ENCODING::OP2);
         inst.op2 = inst.op2
            .ALU_INST(opcode);
      } else if (child->name == "AluOpcode3") {
         const auto name = decodeOpcodeAlias(child->token);
         const auto opcode = latte::getAluOp3InstructionByName(name);

         if (opcode == latte::SQ_OP3_INST_INVALID) {
            throw invalid_alu_op3_inst_exception { *child };
         }

         inst.op3 = inst.op3
            .ALU_INST(opcode);
      } else if (child->name == "AluOutputModifier") {
         inst.op2 = inst.op2
            .OMOD(parseOutputModifier(*child));
      } else if (child->name == "AluDst") {
         for (auto &dst : child->nodes) {
            if (dst->name == "WriteMask") {
               if (inst.word1.ENCODING() != latte::SQ_ALU_ENCODING::OP2) {
                  throw node_parse_exception { *dst, fmt::format("Write mask ____ is only valid on an OP2 instruction") };
               }

               inst.op2 = inst.op2
                  .WRITE_MASK(false);
            } else if (dst->name == "Gpr") {
               inst.word1 = inst.word1
                  .DST_GPR(parseNumber(*dst));
               markGprWritten(shader, inst.word1.DST_GPR());
            } else if (dst->name == "AluRel") {
               inst.word0 = inst.word0
                  .INDEX_MODE(parseAluDstRelIndexMode(*dst));

               inst.word1 = inst.word1
                  .DST_REL(latte::SQ_REL::REL);
            } else if (dst->name == "OneCompSwizzle") {
               inst.word1 = inst.word1
                  .DST_CHAN(parseChan(*dst));
            } else {
               throw unhandled_node_exception { *dst };
            }
         }
      } else if (child->name == "AluSrc") {
         auto negate = false;
         auto srcAbs = false;
         auto rel = latte::SQ_REL::ABS;
         auto chan = inst.word1.DST_CHAN();
         auto sel = latte::SQ_ALU_SRC::REGISTER_FIRST;

         for (auto src : child->nodes) {
            if (src->name == "Negate") {
               negate = true;
            } else if (src->name == "AluRel") {
               rel = latte::SQ_REL::REL;
               inst.word0 = inst.word0
                  .INDEX_MODE(parseAluDstRelIndexMode(*src));
            } else if (src->name == "OneCompSwizzle") {
               chan = parseChan(*src);
            } else if (src->name == "AluAbsSrcValue" || src->name == "AluSrcValue") {
               auto srcType = src->nodes[0];

               if (src->name == "AluAbsSrcValue") {
                  srcAbs = true;
                  srcType = src->nodes[0]->nodes[0];
               }

               if (srcType->name == "Gpr") {
                  sel = static_cast<latte::SQ_ALU_SRC>(latte::SQ_ALU_SRC::REGISTER_FIRST + parseNumber(*srcType));
               } else if (srcType->name == "ConstantCache0") {
                  sel = static_cast<latte::SQ_ALU_SRC>(latte::SQ_ALU_SRC::KCACHE_BANK0_FIRST + parseNumber(*srcType));
               } else if (srcType->name == "ConstantCache1") {
                  sel = static_cast<latte::SQ_ALU_SRC>(latte::SQ_ALU_SRC::KCACHE_BANK1_FIRST + parseNumber(*srcType));
               } else if (srcType->name == "ConstantFile") {
                  sel = static_cast<latte::SQ_ALU_SRC>(latte::SQ_ALU_SRC::CONST_FILE_FIRST + parseNumber(*srcType));
               } else if (srcType->name == "PreviousScalar") {
                  sel = static_cast<latte::SQ_ALU_SRC>(latte::SQ_ALU_SRC::PS);
               } else if (srcType->name == "PreviousVector") {
                  sel = static_cast<latte::SQ_ALU_SRC>(latte::SQ_ALU_SRC::PV);
               } else if (srcType->name == "Literal") {
                  auto literal = parseLiteral(*srcType);
                  sel = latte::SQ_ALU_SRC::LITERAL;

                  if (literal.flags & LiteralValue::ReadFloat) {
                     if (literal.floatValue == 0.0f) {
                        if ((literal.flags & LiteralValue::ReadHex) == 0 || literal.hexValue == 0) {
                           sel = latte::SQ_ALU_SRC::IMM_0;
                        }
                     } else if (literal.floatValue == 1.0f) {
                        sel = latte::SQ_ALU_SRC::IMM_1;
                     } else if (literal.floatValue == 0.5f) {
                        sel = latte::SQ_ALU_SRC::IMM_0_5;
                     }
                  }

                  if (sel == latte::SQ_ALU_SRC::LITERAL) {
                     chan = static_cast<latte::SQ_CHAN>(group.literals.size());
                     group.literals.push_back(literal);
                  }
               } else {
                  throw unhandled_node_exception { *srcType };
               }
            } else {
               throw unhandled_node_exception { *src };
            }
         }

         if (srcIndex == 0) {
            inst.word0 = inst.word0
               .SRC0_CHAN(chan)
               .SRC0_NEG(negate)
               .SRC0_SEL(sel)
               .SRC0_REL(rel);

            if (srcAbs) {
               inst.op2 = inst.op2
                  .SRC0_ABS(true);
            }
         } else if (srcIndex == 1) {
            inst.word0 = inst.word0
               .SRC1_CHAN(chan)
               .SRC1_NEG(negate)
               .SRC1_SEL(sel)
               .SRC1_REL(rel);

            if (srcAbs) {
               inst.op2 = inst.op2
                  .SRC1_ABS(true);
            }
         } else if (srcIndex == 2) {
            inst.op3 = inst.op3
               .SRC2_CHAN(chan)
               .SRC2_NEG(negate)
               .SRC2_SEL(sel)
               .SRC2_REL(rel);
         }

         markSrcRead(shader, sel);
         srcIndex++;
      } else if (child->name == "AluProperties") {
         for (auto &prop : child->nodes) {
            if (prop->name == "BANK_SWIZZLE") {
               inst.word1 = inst.word1
                  .BANK_SWIZZLE(parseAluBankSwizzle(*prop));
            } else if (prop->name == "UPDATE_EXEC_MASK") {
               inst.op2 = inst.op2
                  .UPDATE_EXECUTE_MASK(true);
            } else if (prop->name == "UPDATE_PRED") {
               inst.op2 = inst.op2
                  .UPDATE_PRED(true);
            } else if (prop->name == "PRED_SEL") {
               inst.word0 = inst.word0
                  .PRED_SEL(parsePredSel(*prop));
            } else if (prop->name == "CLAMP") {
               inst.word1 = inst.word1
                  .CLAMP(true);
            } else {
               throw invalid_alu_property_exception { *prop };
            }
         }
      } else {
         throw unhandled_node_exception { *child };
      }
   }

   group.insts.push_back(inst);
}

static void
compileAluGroup(Shader &shader, AluClause &clause, peg::Ast &node)
{
   auto group = AluGroup {};

   for (auto &child : node.nodes) {
      if (child->name == "InstCount") {
         group.clausePC = parseNumber(*child);

         if (group.clausePC != shader.clausePC) {
            throw incorrect_clause_pc_exception { *child, group.clausePC, shader.clausePC };
         }
      } else if (child->name == "AluScalar0") {
         compileAluInst(shader, group, *child, 0);
      } else if (child->name == "AluScalar1") {
         compileAluInst(shader, group, *child, 1);
      } else if (child->name == "AluScalar2") {
         compileAluInst(shader, group, *child, 2);
      } else if (child->name == "AluScalar3") {
         compileAluInst(shader, group, *child, 3);
      } else {
         throw unhandled_node_exception { *child };
      }
   }

   if (group.insts.size()) {
      auto &lastInst = group.insts.back();
      lastInst.word0 = lastInst.word0
         .LAST(true);
   }

   shader.clausePC++;
   clause.groups.push_back(std::move(group));
}

void
compileAluClause(Shader &shader,
                 peg::Ast &node)
{
   auto cfInst = latte::ControlFlowInst { };
   auto clause = AluClause {};

   cfInst.alu.word1 = cfInst.alu.word1
      .BARRIER(true);

   for (auto &child : node.nodes) {
      if (child->name == "InstCount") {
         clause.cfPC = parseNumber(*child);

         if (clause.cfPC != shader.cfInsts.size()) {
            throw incorrect_cf_pc_exception { *child, clause.cfPC, shader.cfInsts.size() };
         }
      } else if (child->name == "AluClauseInstType") {
         auto &name = child->token;
         auto opcode = latte::getCfAluInstructionByName(name);

         if (opcode == latte::SQ_CF_ALU_INST_INVALID) {
            throw invalid_cf_alu_inst_exception { *child };
         }

         cfInst.alu.word1 = cfInst.alu.word1
            .CF_INST(opcode);
      } else if (child->name == "AluClauseProperties") {
         for (auto &prop : child->nodes) {
            if (prop->name == "ADDR") {
               clause.addrNode = prop;
               cfInst.alu.word0 = cfInst.alu.word0
                  .ADDR(parseNumber(*prop));
            } else if (prop->name == "CNT") {
               clause.countNode = prop;
               cfInst.alu.word1 = cfInst.alu.word1
                  .COUNT(parseNumber(*prop) - 1);
            } else if (prop->name == "KCACHE0") {
               assert(prop->nodes.size() == 3);
               auto bank = parseNumber(*prop->nodes[0]);
               auto start = parseNumber(*prop->nodes[1]);
               auto end = parseNumber(*prop->nodes[2]);
               auto mode = latte::SQ_CF_KCACHE_MODE::NOP;

               if (end - start == 15) {
                  mode = latte::SQ_CF_KCACHE_MODE::LOCK_1;
               } else if (end - start == 31) {
                  mode = latte::SQ_CF_KCACHE_MODE::LOCK_2;
               }

               cfInst.alu.word0 = cfInst.alu.word0
                  .KCACHE_BANK0(bank)
                  .KCACHE_MODE0(mode);

               cfInst.alu.word1 = cfInst.alu.word1
                  .KCACHE_ADDR0(start / 16);
            } else if (prop->name == "KCACHE1") {
               assert(prop->nodes.size() == 3);
               auto bank = parseNumber(*prop->nodes[0]);
               auto start = parseNumber(*prop->nodes[1]);
               auto end = parseNumber(*prop->nodes[2]);
               auto mode = latte::SQ_CF_KCACHE_MODE::NOP;

               if (end - start == 15) {
                  mode = latte::SQ_CF_KCACHE_MODE::LOCK_1;
               } else if (end - start == 31) {
                  mode = latte::SQ_CF_KCACHE_MODE::LOCK_2;
               }

               cfInst.alu.word0 = cfInst.alu.word0
                  .KCACHE_BANK1(bank);

               cfInst.alu.word1 = cfInst.alu.word1
                  .KCACHE_MODE1(mode)
                  .KCACHE_ADDR1(start / 16);
            } else if (prop->name == "NO_BARRIER") {
               cfInst.alu.word1 = cfInst.alu.word1
                  .BARRIER(false);
            } else if (prop->name == "WHOLE_QUAD_MODE") {
               cfInst.alu.word1 = cfInst.alu.word1
                  .WHOLE_QUAD_MODE(true);
            } else if (prop->name == "USES_WATERFALL") {
               cfInst.alu.word1 = cfInst.alu.word1
                  .ALT_CONST(true);
            } else {
               throw invalid_cf_alu_property_exception { *prop };
            }
         }
      } else if (child->name == "AluGroup") {
         compileAluGroup(shader, clause, *child);
      } else {
         throw unhandled_node_exception { *child };
      }
   }

   shader.aluClauses.push_back(std::move(clause));
   shader.cfInsts.push_back(cfInst);
}
