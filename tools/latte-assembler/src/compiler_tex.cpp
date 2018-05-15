#include "shader_compiler.h"

static void
compileTexInst(Shader &shader,
               TexClause &clause,
               peg::Ast &node)
{

   auto inst = latte::TextureFetchInst { };
   auto srcIndex = 0;
   std::memset(&inst, 0, sizeof(latte::TextureFetchInst));

   inst.word0 = inst.word0
      .SRC_REL(latte::SQ_REL::ABS);

   inst.word1 = inst.word1
      .DST_REL(latte::SQ_REL::ABS)
      .DST_SEL_X(latte::SQ_SEL::SEL_X)
      .DST_SEL_Y(latte::SQ_SEL::SEL_Y)
      .DST_SEL_Z(latte::SQ_SEL::SEL_Z)
      .DST_SEL_W(latte::SQ_SEL::SEL_W);

   inst.word2 = inst.word2
      .SRC_SEL_X(latte::SQ_SEL::SEL_X)
      .SRC_SEL_Y(latte::SQ_SEL::SEL_Y)
      .SRC_SEL_Z(latte::SQ_SEL::SEL_Z)
      .SRC_SEL_W(latte::SQ_SEL::SEL_W);

   inst.word1 = inst.word1
      .COORD_TYPE_X(latte::SQ_TEX_COORD_TYPE::NORMALIZED)
      .COORD_TYPE_Y(latte::SQ_TEX_COORD_TYPE::NORMALIZED)
      .COORD_TYPE_Z(latte::SQ_TEX_COORD_TYPE::NORMALIZED)
      .COORD_TYPE_W(latte::SQ_TEX_COORD_TYPE::NORMALIZED);

   for (auto &child : node.nodes) {
      if (child->name == "InstCount") {
         auto clausePC = parseNumber(*child);

         if (clausePC != shader.clausePC) {
            throw incorrect_clause_pc_exception { *child, clausePC, shader.clausePC };
         }

         shader.clausePC++;
      } else if (child->name == "TexOpcode") {
         auto &name = child->token;
         auto opcode = latte::getTexInstructionByName(name);

         if (opcode == latte::SQ_TEX_INST_INVALID) {
            throw invalid_tex_inst_exception { *child };
         }

         inst.word0 = inst.word0
            .TEX_INST(opcode);
      } else if (child->name == "TexDst") {
         for (auto &dst : child->nodes) {
            if (dst->name == "WriteMask") {
               inst.word1 = inst.word1
                  .DST_SEL_X(latte::SQ_SEL::SEL_MASK)
                  .DST_SEL_Y(latte::SQ_SEL::SEL_MASK)
                  .DST_SEL_Z(latte::SQ_SEL::SEL_MASK)
                  .DST_SEL_W(latte::SQ_SEL::SEL_MASK);
            } else if (dst->name == "Gpr") {
               inst.word1 = inst.word1
                  .DST_GPR(parseNumber(*dst));
               markGprWritten(shader, inst.word1.DST_GPR());
            } else if (dst->name == "TexRel") {
               inst.word1 = inst.word1
                  .DST_REL(latte::SQ_REL::REL);
            } else if (dst->name == "FourCompSwizzle") {
               auto selX = latte::SQ_SEL::SEL_X;
               auto selY = latte::SQ_SEL::SEL_Y;
               auto selZ = latte::SQ_SEL::SEL_Z;
               auto selW = latte::SQ_SEL::SEL_W;

               parseFourCompSwizzle(*dst, selX, selY, selZ, selW);
               inst.word1 = inst.word1
                  .DST_SEL_X(selX)
                  .DST_SEL_Y(selY)
                  .DST_SEL_Z(selZ)
                  .DST_SEL_W(selW);
            } else {
               throw unhandled_node_exception { *dst };
            }
         }
      } else if (child->name == "TexSrc") {
         for (auto &src : child->nodes) {
            if (src->name == "Gpr") {
               inst.word0 = inst.word0
                  .SRC_GPR(parseNumber(*src));
               markGprRead(shader, inst.word0.SRC_GPR());
            } else if (src->name == "TexRel") {
               inst.word0 = inst.word0
                  .SRC_REL(latte::SQ_REL::REL);
            } else if (src->name == "FourCompSwizzle") {
               auto selX = latte::SQ_SEL::SEL_X;
               auto selY = latte::SQ_SEL::SEL_Y;
               auto selZ = latte::SQ_SEL::SEL_Z;
               auto selW = latte::SQ_SEL::SEL_W;

               parseFourCompSwizzle(*src, selX, selY, selZ, selW);
               inst.word2 = inst.word2
                  .SRC_SEL_X(selX)
                  .SRC_SEL_Y(selY)
                  .SRC_SEL_Z(selZ)
                  .SRC_SEL_W(selW);
            } else {
               throw unhandled_node_exception { *src };
            }
         }
      } else if (child->name == "TexResourceId") {
         inst.word0 = inst.word0
            .RESOURCE_ID(parseNumber(*child));
      } else if (child->name == "TexSamplerId") {
         inst.word2 = inst.word2
            .SAMPLER_ID(parseNumber(*child));
      } else if (child->name == "TexProperties") {
         for (auto &prop : child->nodes) {
            if (prop->name == "ALT_CONST") {
               inst.word0 = inst.word0
                  .ALT_CONST(true);
            } else if (prop->name == "BC_FRAC_MODE") {
               inst.word0 = inst.word0
                  .BC_FRAC_MODE(true);
            } else if (prop->name == "DENORM") {
               if (prop->token.find_first_of('x') != std::string::npos) {
                  inst.word1 = inst.word1
                     .COORD_TYPE_X(latte::SQ_TEX_COORD_TYPE::UNNORMALIZED);
               }

               if (prop->token.find_first_of('y') != std::string::npos) {
                  inst.word1 = inst.word1
                     .COORD_TYPE_Y(latte::SQ_TEX_COORD_TYPE::UNNORMALIZED);
               }

               if (prop->token.find_first_of('z') != std::string::npos) {
                  inst.word1 = inst.word1
                     .COORD_TYPE_Z(latte::SQ_TEX_COORD_TYPE::UNNORMALIZED);
               }

               if (prop->token.find_first_of('w') != std::string::npos) {
                  inst.word1 = inst.word1
                     .COORD_TYPE_W(latte::SQ_TEX_COORD_TYPE::UNNORMALIZED);
               }
            } else if (prop->name == "NORM") {
               if (prop->token.find_first_of('x') != std::string::npos) {
                  inst.word1 = inst.word1
                     .COORD_TYPE_X(latte::SQ_TEX_COORD_TYPE::NORMALIZED);
               }

               if (prop->token.find_first_of('y') != std::string::npos) {
                  inst.word1 = inst.word1
                     .COORD_TYPE_Y(latte::SQ_TEX_COORD_TYPE::NORMALIZED);
               }

               if (prop->token.find_first_of('z') != std::string::npos) {
                  inst.word1 = inst.word1
                     .COORD_TYPE_Z(latte::SQ_TEX_COORD_TYPE::NORMALIZED);
               }

               if (prop->token.find_first_of('w') != std::string::npos) {
                  inst.word1 = inst.word1
                     .COORD_TYPE_W(latte::SQ_TEX_COORD_TYPE::NORMALIZED);
               }
            } else if (prop->name == "LOD") {
               inst.word1 = inst.word1
                  .LOD_BIAS(sfixed_1_3_3_t { parseFloat(*prop) });
            } else if (prop->name == "WHOLE_QUAD_MODE") {
               inst.word0 = inst.word0
                  .FETCH_WHOLE_QUAD(true);
            } else if (prop->name == "XOFFSET") {
               inst.word2 = inst.word2
                  .OFFSET_X(sfixed_1_3_1_t { parseFloat(*prop) });
            } else if (prop->name == "YOFFSET") {
               inst.word2 = inst.word2
                  .OFFSET_Y(sfixed_1_3_1_t { parseFloat(*prop) });
            } else if (prop->name == "ZOFFSET") {
               inst.word2 = inst.word2
                  .OFFSET_Z(sfixed_1_3_1_t { parseFloat(*prop) });
            } else {
               throw invalid_tex_property_exception { *prop };
            }
         }
      } else {
         throw unhandled_node_exception { *child };
      }
   }

   clause.insts.push_back(inst);
}

void
compileTexClause(Shader &shader,
                 peg::Ast &node)
{
   auto cfInst = latte::ControlFlowInst { };
   auto clause = TexClause {};
   clause.clausePC = shader.clausePC;

   cfInst.word1 = cfInst.word1
      .BARRIER(true);

   for (auto &child : node.nodes) {
      if (child->name == "InstCount") {
         clause.cfPC = parseNumber(*child);

         if (clause.cfPC != shader.cfInsts.size()) {
            throw incorrect_cf_pc_exception { *child, clause.cfPC, shader.cfInsts.size() };
         }
      } else if (child->name == "TexClauseInstType") {
         auto &name = child->token;
         auto opcode = latte::getCfInstructionByName(name);

         if (opcode == latte::SQ_CF_INST_INVALID) {
            throw invalid_cf_tex_inst_exception { *child };
         }

         cfInst.word1 = cfInst.word1
            .CF_INST_TYPE(latte::SQ_CF_INST_TYPE_NORMAL)
            .CF_INST(opcode);
      } else if (child->name == "TexClauseProperties") {
         for (auto &prop : child->nodes) {
            if (prop->name == "ADDR") {
               clause.addrNode = prop;
               cfInst.word0 = cfInst.word0
                  .ADDR(parseNumber(*prop));
            } else if (prop->name == "CNT") {
               auto count = parseNumber(*prop) - 1;
               clause.countNode = prop;
               cfInst.word1 = cfInst.word1
                  .COUNT_3(count >> 3)
                  .COUNT(count & 0b111);
            } else if (prop->name == "CND") {
               cfInst.word1 = cfInst.word1
                  .COND(parseCfCond(*prop));
            } else if (prop->name == "CF_CONST") {
               cfInst.word1 = cfInst.word1
                  .CF_CONST(parseNumber(*prop));
            } else if (prop->name == "NO_BARRIER") {
               cfInst.word1 = cfInst.word1
                  .BARRIER(false);
            } else if (prop->name == "WHOLE_QUAD_MODE") {
               cfInst.word1 = cfInst.word1
                  .WHOLE_QUAD_MODE(true);
            } else if (prop->name == "VALID_PIX") {
               cfInst.word1 = cfInst.word1
                  .VALID_PIXEL_MODE(true);
            } else {
               throw invalid_cf_tex_property_exception { *prop };
            }
         }
      } else if (child->name == "TexInst") {
         compileTexInst(shader, clause, *child);
      } else {
         throw unhandled_node_exception { *child };
      }
   }

   shader.texClauses.push_back(std::move(clause));
   shader.cfInsts.push_back(cfInst);
}
