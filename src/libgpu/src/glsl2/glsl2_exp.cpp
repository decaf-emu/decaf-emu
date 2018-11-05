#include "glsl2_translate.h"
#include "latte/latte_instructions.h"

#include <common/log.h>
#include <fmt/format.h>

using namespace latte;

/*
Unimplemented:
MEM_SCRATCH
MEM_REDUCTION
MEM_RING
MEM_EXPORT
*/

namespace glsl2
{

void
insertExportRegister(fmt::memory_buffer &out, uint32_t gpr, SQ_REL rel)
{
   fmt::format_to(out, "R[{}", gpr);

   if (rel) {
      fmt::format_to(out, " + AL");
   }

   fmt::format_to(out, "]");
}

std::string
getExportRegister(uint32_t gpr, SQ_REL rel)
{
   fmt::memory_buffer out;
   insertExportRegister(out, gpr, rel);
   return to_string(out);
}

bool
insertSelectValue(fmt::memory_buffer &out, const std::string &src, SQ_SEL sel)
{
   switch (sel) {
   case SQ_SEL::SEL_X:
      fmt::format_to(out, "{}.x", src);
      break;
   case SQ_SEL::SEL_Y:
      fmt::format_to(out, "{}.y", src);
      break;
   case SQ_SEL::SEL_Z:
      fmt::format_to(out, "{}.z", src);
      break;
   case SQ_SEL::SEL_W:
      fmt::format_to(out, "{}.w", src);
      break;
   case SQ_SEL::SEL_0:
      fmt::format_to(out, "0");
      break;
   case SQ_SEL::SEL_1:
      fmt::format_to(out, "1");
      break;
   case SQ_SEL::SEL_MASK:
      // These should never show up since if it does, it means that need
      //  to actually do a condensing first and adjust the target swizzle.
      throw translate_exception("Unexpected SQ_SEL::SEL_MASK");
   default:
      throw translate_exception(fmt::format("Unexpected SQ_SEL value {}", sel));
   }

   return true;
}

bool
insertSelectVector(fmt::memory_buffer &out, const std::string &src, SQ_SEL selX, SQ_SEL selY, SQ_SEL selZ, SQ_SEL selW, unsigned numSels)
{
   SQ_SEL sels[4] = { selX, selY, selZ, selW };

   if (numSels == 1) {
      insertSelectValue(out, src, sels[0]);
   } else {
      auto isTrivialSwizzle = true;

      for (auto i = 0u; i < numSels; ++i) {
         if (sels[i] != SQ_SEL::SEL_X && sels[i] != SQ_SEL::SEL_Y && sels[i] != SQ_SEL::SEL_Z && sels[i] != SQ_SEL::SEL_W) {
            isTrivialSwizzle = false;
         }
      }

      if (isTrivialSwizzle) {
         fmt::format_to(out, "{}.", src);

         for (auto i = 0u; i < numSels; ++i) {
            switch (sels[i]) {
            case SQ_SEL::SEL_X:
               fmt::format_to(out, "x");
               break;
            case SQ_SEL::SEL_Y:
               fmt::format_to(out, "y");
               break;
            case SQ_SEL::SEL_Z:
               fmt::format_to(out, "z");
               break;
            case SQ_SEL::SEL_W:
               fmt::format_to(out, "w");
               break;
            }
         }
      } else {
         fmt::format_to(out, "vec{}(", numSels);

         insertSelectValue(out, src, sels[0]);

         if (numSels >= 2) {
            fmt::format_to(out, ", ");
            insertSelectValue(out, src, sels[1]);
         }

         if (numSels >= 3) {
            fmt::format_to(out, ", ");
            insertSelectValue(out, src, sels[2]);
         }

         if (numSels >= 4) {
            fmt::format_to(out, ", ");
            insertSelectValue(out, src, sels[3]);
         }

         fmt::format_to(out, ")");
      }
   }

   return true;
}

std::string
condenseSelections(SQ_SEL &selX, SQ_SEL &selY, SQ_SEL &selZ, SQ_SEL &selW, unsigned &numSels)
{
   std::string value;
   value.reserve(4);
   auto numSelsOut = 0u;
   SQ_SEL sels[4] = { selX, selY, selZ, selW };

   for (auto i = 0u; i < numSels; ++i) {
      if (sels[i] != SQ_SEL::SEL_MASK) {
         sels[numSelsOut] = sels[i];
         numSelsOut++;

         if (i == 0) {
            value.push_back('x');
         } else if (i == 1) {
            value.push_back('y');
         } else if (i == 2) {
            value.push_back('z');
         } else if (i == 3) {
            value.push_back('w');
         }
      }
   }

   selX = sels[0];
   selY = sels[1];
   selZ = sels[2];
   selW = sels[3];
   numSels = numSelsOut;
   return value;
}

bool
insertMaskVector(fmt::memory_buffer &out, const std::string &src, unsigned mask)
{
   SQ_SEL selX = mask & (1 << 0) ? SQ_SEL::SEL_X : SQ_SEL::SEL_MASK;
   SQ_SEL selY = mask & (1 << 1) ? SQ_SEL::SEL_Y : SQ_SEL::SEL_MASK;
   SQ_SEL selZ = mask & (1 << 2) ? SQ_SEL::SEL_Z : SQ_SEL::SEL_MASK;
   SQ_SEL selW = mask & (1 << 3) ? SQ_SEL::SEL_W : SQ_SEL::SEL_MASK;

   auto numSels = 4u;
   condenseSelections(selX, selY, selZ, selW, numSels);

   return insertSelectVector(out, src, selX, selY, selZ, selW, numSels);
}

static void
registerExport(State &state, SQ_EXPORT_TYPE type, unsigned arrayBase)
{
   Export exp;
   exp.type = type;

   if (type == SQ_EXPORT_TYPE::POS) {
      exp.id = arrayBase - 60;
   } else {
      exp.id = arrayBase;
   }

   if (state.shader) {
      state.shader->exports.push_back(exp);
   }
}

static void
registerFeedback(State &state,
                 unsigned streamIndex,
                 unsigned offset,
                 unsigned size)
{
   if (state.shader) {
      Feedback xfb;
      xfb.streamIndex = streamIndex;
      xfb.offset = offset;
      xfb.size = size;
      state.shader->feedbacks[streamIndex].push_back(xfb);
   }
}

static void
EXP(State &state, const ControlFlowInst &cf)
{
   auto type = cf.exp.word0.TYPE();
   auto arrayBase = cf.exp.word0.ARRAY_BASE();

   auto selX = cf.exp.swiz.SEL_X();
   auto selY = cf.exp.swiz.SEL_Y();
   auto selZ = cf.exp.swiz.SEL_Z();
   auto selW = cf.exp.swiz.SEL_W();

   if (selX == SQ_SEL::SEL_MASK && selY == SQ_SEL::SEL_MASK && selZ == SQ_SEL::SEL_MASK && selW == SQ_SEL::SEL_MASK) {
      gLog->warn("Unusual shader with a fully masked export");
      return;
   }

   auto numSrcSels = 4u;
   auto srcSelMask = condenseSelections(selX, selY, selZ, selW, numSrcSels);

   for (auto i = 0u; i <= cf.exp.word1.BURST_COUNT(); ++i) {
      auto outIndex = arrayBase + i;
      auto src = getExportRegister(cf.exp.word0.RW_GPR() + i, cf.exp.word0.RW_REL());

      registerExport(state, type, outIndex);
      insertLineStart(state);

      switch (type) {
      case SQ_EXPORT_TYPE::POS:
         fmt::format_to(state.out, "exp_position_{}", outIndex - 60);
         break;
      case SQ_EXPORT_TYPE::PARAM:
         fmt::format_to(state.out, "exp_param_{}", outIndex);
         break;
      case SQ_EXPORT_TYPE::PIXEL:
         fmt::format_to(state.out, "exp_pixel_{}", outIndex);
         break;
      default:
         throw translate_exception(fmt::format("Unsupported export type {}", type));
      }

      fmt::format_to(state.out, ".{} = ", srcSelMask);
      insertSelectVector(state.out, src, selX, selY, selZ, selW, numSrcSels);
      fmt::format_to(state.out, ";");

      insertLineEnd(state);
   }
}

static void
MEM_STREAM(State &state, const ControlFlowInst &cf)
{
   auto streamIndex = cf.exp.word1.CF_INST() - latte::SQ_CF_INST_MEM_STREAM0;
   auto type = cf.exp.word0.TYPE();
   auto offset = cf.exp.word0.ARRAY_BASE() * 4;
   auto valueSize = cf.exp.buf.ARRAY_SIZE() + 1;
   auto src = getExportRegister(cf.exp.word0.RW_GPR(), cf.exp.word0.RW_REL());

   switch (static_cast<SQ_MEM_EXPORT_TYPE>(type)) {
   case SQ_MEM_EXPORT_TYPE::WRITE:
      break;
   case SQ_MEM_EXPORT_TYPE::WRITE_IND:
      throw translate_exception(fmt::format("Unsupported EXPORT_WRITE_IND in MEM_STREAM{}", streamIndex));
   default:
      throw translate_exception(fmt::format("Invalid export type {} for MEM_STREAM{}", type, streamIndex));
   }

   if (valueSize > 4) {
      throw translate_exception(fmt::format("Unsupported value size {} in MEM_STREAM{}", valueSize, streamIndex));
   }

   registerFeedback(state, streamIndex, offset, valueSize);

   insertLineStart(state);

   fmt::format_to(state.out, "feedback_{}_{} = ", streamIndex, offset);
   insertMaskVector(state.out, src, cf.exp.buf.COMP_MASK());
   fmt::format_to(state.out, ";");

   insertLineEnd(state);
}

void
registerExpFunctions()
{
   registerInstruction(latte::SQ_CF_INST_EXP, EXP);
   registerInstruction(latte::SQ_CF_INST_EXP_DONE, EXP);
   registerInstruction(latte::SQ_CF_INST_MEM_STREAM0, MEM_STREAM);
   registerInstruction(latte::SQ_CF_INST_MEM_STREAM1, MEM_STREAM);
   registerInstruction(latte::SQ_CF_INST_MEM_STREAM2, MEM_STREAM);
   registerInstruction(latte::SQ_CF_INST_MEM_STREAM3, MEM_STREAM);
}

} // namespace glsl2
