#include "common/log.h"
#include "glsl2_translate.h"
#include "gpu/microcode/latte_instructions.h"

using namespace latte;

/*
Unimplemented:
MEM_STREAM0
MEM_STREAM1
MEM_STREAM2
MEM_STREAM3
MEM_SCRATCH
MEM_REDUCTION
MEM_RING
MEM_EXPORT
*/

namespace glsl2
{

void
insertExportRegister(fmt::MemoryWriter &out, uint32_t gpr, SQ_REL rel)
{
   out << "R[" << gpr;

   if (rel) {
      out << " + AL";
   }

   out << "]";
}

std::string
getExportRegister(uint32_t gpr, SQ_REL rel)
{
   fmt::MemoryWriter out;
   insertExportRegister(out, gpr, rel);
   return out.str();
}

static bool
insertSelectValue(fmt::MemoryWriter &out, const std::string &src, SQ_SEL sel)
{
   switch (sel) {
   case SQ_SEL_X:
      out << src << ".x";
      break;
   case SQ_SEL_Y:
      out << src << ".y";
      break;
   case SQ_SEL_Z:
      out << src << ".z";
      break;
   case SQ_SEL_W:
      out << src << ".w";
      break;
   case SQ_SEL_0:
      out << "0";
      break;
   case SQ_SEL_1:
      out << "1";
      break;
   case SQ_SEL_MASK:
      // These should never show up since if it does, it means that need
      //  to actually do a condensing first and adjust the target swizzle.
      throw translate_exception("Unexpected SQ_SEL_MASK");
   default:
      throw translate_exception(fmt::format("Unexpected SQ_SEL value {}", sel));
   }

   return true;
}

bool
insertSelectVector(fmt::MemoryWriter &out, const std::string &src, SQ_SEL selX, SQ_SEL selY, SQ_SEL selZ, SQ_SEL selW, unsigned numSels)
{
   SQ_SEL sels[4] = { selX, selY, selZ, selW };

   if (numSels == 1) {
      insertSelectValue(out, src, sels[0]);
   } else {
      auto isTrivialSwizzle = true;

      for (auto i = 0u; i < numSels; ++i) {
         if (sels[i] != SQ_SEL_X && sels[i] != SQ_SEL_Y && sels[i] != SQ_SEL_Z && sels[i] != SQ_SEL_W) {
            isTrivialSwizzle = false;
         }
      }

      if (isTrivialSwizzle) {

         out << src << ".";
         for (auto i = 0u; i < numSels; ++i) {
            switch (sels[i]) {
            case SQ_SEL_X:
               out << "x";
               break;
            case SQ_SEL_Y:
               out << "y";
               break;
            case SQ_SEL_Z:
               out << "z";
               break;
            case SQ_SEL_W:
               out << "w";
               break;
            }
         }
      } else {
         out << "vec" << numSels << "(";

         insertSelectValue(out, src, sels[0]);

         if (numSels >= 2) {
            out << ", ";
            insertSelectValue(out, src, sels[1]);
         }

         if (numSels >= 3) {
            out << ", ";
            insertSelectValue(out, src, sels[2]);
         }

         if (numSels >= 4) {
            out << ", ";
            insertSelectValue(out, src, sels[3]);
         }

         out << ")";
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
      if (sels[i] != SQ_SEL_MASK) {
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

static void
registerExport(State &state, SQ_EXPORT_TYPE type, unsigned arrayBase)
{
   Export exp;
   exp.type = type;

   if (type == SQ_EXPORT_POS) {
      exp.id = arrayBase - 60;
   } else {
      exp.id = arrayBase;
   }

   if (state.shader) {
      state.shader->exports.push_back(exp);
   }
}

static void
EXP(State &state, const ControlFlowInst &cf)
{
   auto type = cf.exp.word0.TYPE();
   auto arrayBase = cf.exp.word0.ARRAY_BASE();

   auto selX = cf.exp.swiz.SRC_SEL_X();
   auto selY = cf.exp.swiz.SRC_SEL_Y();
   auto selZ = cf.exp.swiz.SRC_SEL_Z();
   auto selW = cf.exp.swiz.SRC_SEL_W();
   auto src = getExportRegister(cf.exp.word0.RW_GPR(), cf.exp.word0.RW_REL());

   if (selX == SQ_SEL_MASK && selY == SQ_SEL_MASK && selZ == SQ_SEL_MASK && selW == SQ_SEL_MASK) {
      gLog->warn("Unusual shader with a fully masked export");
      return;
   }

   if (selX == SQ_SEL_MASK || selY == SQ_SEL_MASK || selZ == SQ_SEL_MASK || selW == SQ_SEL_MASK) {
      // It doesn't really ever make sense for exports to use masking, so lets make
      //  sure to crash in this instance.  If there is a valid use-case for this, we
      //  just need to switch back to using condenseSelections instead.
      throw translate_exception("Masking exports is non-sense");
   }

   registerExport(state, type, arrayBase);
   insertLineStart(state);

   switch (type) {
   case SQ_EXPORT_POS:
      state.out << "exp_position_" << (arrayBase - 60);
      break;
   case SQ_EXPORT_PARAM:
      state.out << "exp_param_" << arrayBase;
      break;
   case SQ_EXPORT_PIXEL:
      state.out << "exp_pixel_" << arrayBase;
      break;
   default:
      throw translate_exception(fmt::format("Unsupported export type {}", cf.exp.word0.TYPE()));
   }

   state.out << " = ";
   insertSelectVector(state.out, src, selX, selY, selZ, selW, 4);
   state.out << ";";

   insertLineEnd(state);
}

void
registerExpFunctions()
{
   registerInstruction(latte::SQ_CF_INST_EXP, EXP);
   registerInstruction(latte::SQ_CF_INST_EXP_DONE, EXP);
}

} // namespace glsl2
