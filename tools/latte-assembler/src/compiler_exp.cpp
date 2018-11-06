#include "shader_compiler.h"

void
compileExpInst(Shader &shader, peg::Ast &node)
{
   auto inst = latte::ControlFlowInst { };

   inst.word1 = inst.word1
      .CF_INST_TYPE(latte::SQ_CF_INST_TYPE_EXPORT)
      .BARRIER(true);

   for (auto &child : node.nodes) {
      if (child->name == "InstCount") {
         auto cfPC = parseNumber(*child);

         if (cfPC != shader.cfInsts.size()) {
            throw incorrect_cf_pc_exception { *child, cfPC, shader.cfInsts.size() };
         }
      } else if (child->name == "ExpOpcode") {
         auto &name = child->token;
         auto opcode = latte::getCfExpInstructionByName(name);

         if (opcode == latte::SQ_CF_EXP_INST_INVALID) {
            throw invalid_exp_inst_exception { *child };
         }

         if (opcode != latte::SQ_CF_INST_EXP && opcode != latte::SQ_CF_INST_EXP_DONE) {
            // TODO: Only EXP and EXP_DONE are supported for now.
            throw invalid_exp_inst_exception { *child };
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
            if (src->name == "Gpr") {
               inst.exp.word0 = inst.exp.word0
                  .RW_GPR(parseNumber(*src))
                  .RW_REL(latte::SQ_REL::ABS);

               // Set default GPR swizzle
               inst.exp.swiz = inst.exp.swiz
                  .SEL_X(latte::SQ_SEL::SEL_X)
                  .SEL_Y(latte::SQ_SEL::SEL_Y)
                  .SEL_Z(latte::SQ_SEL::SEL_Z)
                  .SEL_W(latte::SQ_SEL::SEL_W);

               markGprRead(shader, inst.exp.word0.RW_GPR());
            } else if (src->name == "GprRel") {
               inst.exp.word0 = inst.exp.word0
                  .RW_GPR(parseNumber(*src))
                  .RW_REL(latte::SQ_REL::REL);

               // Set default GPR swizzle
               inst.exp.swiz = inst.exp.swiz
                  .SEL_X(latte::SQ_SEL::SEL_X)
                  .SEL_Y(latte::SQ_SEL::SEL_Y)
                  .SEL_Z(latte::SQ_SEL::SEL_Z)
                  .SEL_W(latte::SQ_SEL::SEL_W);

               markGprRead(shader, inst.exp.word0.RW_GPR());
            } else if (src->name == "FourCompSwizzle") {
               auto selX = latte::SQ_SEL::SEL_0;
               auto selY = latte::SQ_SEL::SEL_0;
               auto selZ = latte::SQ_SEL::SEL_0;
               auto selW = latte::SQ_SEL::SEL_1;

               parseFourCompSwizzle(*src, selX, selY, selZ, selW);
               inst.exp.swiz = inst.exp.swiz
                  .SEL_X(selX)
                  .SEL_Y(selY)
                  .SEL_Z(selZ)
                  .SEL_W(selW);
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
               throw invalid_exp_property_exception { *prop };
            }
         }
      } else {
         throw unhandled_node_exception { *child };
      }
   }

   shader.cfInsts.push_back(inst);
}
