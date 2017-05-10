#include "shader_compiler.h"

bool
compileExpInst(Shader &shader, peg::Ast &node)
{
   auto inst = latte::ControlFlowInst { 0 };

   inst.word1 = inst.word1
      .CF_INST_TYPE(latte::SQ_CF_INST_TYPE_EXPORT)
      .BARRIER(true);

   for (auto &child : node.nodes) {
      if (child->name == "InstCount") {
         auto cfPC = parseNumber(*child);

         if (cfPC != shader.cfInsts.size()) {
            throw parse_exception(fmt::format("{}:{} Incorrect CF PC {}, expected {}", child->line, child->column, cfPC, shader.cfInsts.size()));
         }
      } else if (child->name == "ExpOpcode") {
         auto &name = child->token;
         auto opcode = latte::getCfExpInstructionByName(name);

         if (opcode == latte::SQ_CF_EXP_INST_INVALID) {
            throw parse_exception(fmt::format("Invalid CF EXP instruction name {}", name));
         }

         if (opcode != latte::SQ_CF_INST_EXP && opcode != latte::SQ_CF_INST_EXP_DONE) {
            throw parse_exception(fmt::format("Unsupported CF EXP instruction name {}", name));
         }

         inst.exp.word1 = inst.exp.word1.CF_INST(opcode);
      } else if (child->name == "ExpParamTarget") {
         auto index = parseNumber(*child);
         shader.maxParamExport = std::max(shader.maxParamExport, index);

         inst.exp.word0 = inst.exp.word0
            .TYPE(latte::SQ_EXPORT_TYPE::PARAM)
            .ARRAY_BASE(index);
      } else if (child->name == "ExpPixTarget") {
         auto index = parseNumber(*child);
         shader.maxPixelExport = std::max(shader.maxPixelExport, index);

         inst.exp.word0 = inst.exp.word0
            .TYPE(latte::SQ_EXPORT_TYPE::PIXEL)
            .ARRAY_BASE(index);
      } else if (child->name == "ExpPosTarget") {
         auto index = parseNumber(*child);
         shader.maxPosExport = std::max(shader.maxPosExport, index);

         inst.exp.word0 = inst.exp.word0
            .TYPE(latte::SQ_EXPORT_TYPE::POS)
            .ARRAY_BASE(index + 60);
      } else if (child->name == "ExpSrc") {
         for (auto &src : child->nodes) {
            if (src->name == "SrcGpr") {
               inst.exp.word0 = inst.exp.word0
                  .RW_GPR(parseNumber(*src))
                  .RW_REL(latte::SQ_REL::ABS);

               // Set default GPR swizzle
               inst.exp.swiz = inst.exp.swiz
                  .SRC_SEL_X(latte::SQ_SEL::SEL_X)
                  .SRC_SEL_Y(latte::SQ_SEL::SEL_Y)
                  .SRC_SEL_Z(latte::SQ_SEL::SEL_Z)
                  .SRC_SEL_W(latte::SQ_SEL::SEL_W);
            } else if (src->name == "SrcGprRel") {
               inst.exp.word0 = inst.exp.word0
                  .RW_GPR(parseNumber(*src))
                  .RW_REL(latte::SQ_REL::REL);

               // Set default GPR swizzle
               inst.exp.swiz = inst.exp.swiz
                  .SRC_SEL_X(latte::SQ_SEL::SEL_X)
                  .SRC_SEL_Y(latte::SQ_SEL::SEL_Y)
                  .SRC_SEL_Z(latte::SQ_SEL::SEL_Z)
                  .SRC_SEL_W(latte::SQ_SEL::SEL_W);
            } else if (src->name == "FourCompSwizzle") {
               auto selX = latte::SQ_SEL::SEL_0;
               auto selY = latte::SQ_SEL::SEL_0;
               auto selZ = latte::SQ_SEL::SEL_0;
               auto selW = latte::SQ_SEL::SEL_1;

               parseFourCompSwizzle(*src, selX, selY, selZ, selW);
               inst.exp.swiz = inst.exp.swiz
                  .SRC_SEL_X(selX)
                  .SRC_SEL_Y(selY)
                  .SRC_SEL_Z(selZ)
                  .SRC_SEL_W(selW);
            }
         }
      } else if (child->name == "CfInstProperties") {
         for (auto &prop : child->nodes) {
            if (prop->name == "NO_BARRIER") {
               inst.word1 = inst.word1.BARRIER(false);
            } else if (prop->name == "WHOLE_QUAD_MODE") {
               inst.exp.word1 = inst.exp.word1.WHOLE_QUAD_MODE(true);
            } else if (prop->name == "VALID_PIX") {
               inst.exp.word1 = inst.exp.word1.VALID_PIXEL_MODE(true);
            } else if (prop->name == "ELEM_SIZE") {
               inst.exp.word0 = inst.exp.word0.ELEM_SIZE(parseNumber(*prop));
            } else if (prop->name == "BURSTCNT") {
               inst.exp.word1 = inst.exp.word1.BURST_COUNT(parseNumber(*prop));
            } else {
               throw parse_exception(fmt::format("{}:{} Unexpected CF property {}", prop->line, prop->column, prop->name));
            }
         }
      } else {
         throw parse_exception(fmt::format("{}:{} Unexpected node {}", child->line, child->column, child->name));
      }
   }

   shader.cfInsts.push_back(inst);
   return true;
}
