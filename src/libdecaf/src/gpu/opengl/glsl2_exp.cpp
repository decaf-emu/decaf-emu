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
   if (gpr >= latte::SQ_ALU_TMP_REGISTER_FIRST) {
      out << "T[";
   } else {
      out << "R[";
   }

   out << gpr;

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

std::string
getSelectDestinationMask(SQ_SEL x, SQ_SEL y, SQ_SEL z, SQ_SEL w)
{
   std::string value;
   value.reserve(4);

   if (x != SQ_SEL_MASK) {
      value.push_back('x');
   }

   if (y != SQ_SEL_MASK) {
      value.push_back('y');
   }

   if (z != SQ_SEL_MASK) {
      value.push_back('z');
   }

   if (w != SQ_SEL_MASK) {
      value.push_back('w');
   }

   return value;
}

bool
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
      return false;
   default:
      throw std::logic_error(fmt::format("Unexpected SQ_SEL value {}", sel));
   }

   return true;
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
      throw std::logic_error(fmt::format("Unsupported export type {}", cf.exp.word0.TYPE()));
   }

   auto selX = cf.exp.swiz.SRC_SEL_X();
   auto selY = cf.exp.swiz.SRC_SEL_Y();
   auto selZ = cf.exp.swiz.SRC_SEL_Z();
   auto selW = cf.exp.swiz.SRC_SEL_W();
   auto src = getExportRegister(cf.exp.word0.RW_GPR(), cf.exp.word0.RW_REL());

   auto dstMask = getSelectDestinationMask(selX, selY, selZ, selW);
   state.out << "." << dstMask << " = ";

   state.out << "vec" << dstMask.length() << "(";

   if (insertSelectValue(state.out, src, selX)) {
      state.out << ", ";
   }

   if (insertSelectValue(state.out, src, selY)) {
      state.out << ", ";
   }

   if (insertSelectValue(state.out, src, selZ)) {
      state.out << ", ";
   }

   insertSelectValue(state.out, src, selW);
   state.out << ");";

   insertLineEnd(state);
}

void
registerExpFunctions()
{
   registerInstruction(latte::SQ_CF_INST_EXP, EXP);
   registerInstruction(latte::SQ_CF_INST_EXP_DONE, EXP);
}

} // namespace glsl2
