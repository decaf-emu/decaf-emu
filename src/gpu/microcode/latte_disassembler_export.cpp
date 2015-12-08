#include "latte_disassembler.h"

namespace latte
{

namespace disassembler
{

static char
disassembleDestMask(SQ_SEL sel)
{
   switch (sel) {
   case SQ_SEL_X:
      return 'x';
   case SQ_SEL_Y:
      return 'y';
   case SQ_SEL_Z:
      return 'z';
   case SQ_SEL_W:
      return 'w';
   case SQ_SEL_0:
      return '0';
   case SQ_SEL_1:
      return '1';
   case SQ_SEL_MASK:
      return '_';
   default:
      return '?';
   }
}

bool
disassembleExport(State &state, shadir::ExportInstruction *inst)
{
   state.out.write("{}{:02} {}", state.indent, inst->cfPC, inst->name);

   if (inst->isSemantic) {
      switch (inst->exportType) {
      case SQ_EXPORT_PIXEL:
         state.out << " PIXEL" << inst->arrayBase;
         break;
      case SQ_EXPORT_POS:
         state.out << " POS" << (inst->arrayBase - 60);
         break;
      case SQ_EXPORT_PARAM:
         state.out << " PARAM" << inst->arrayBase;
         break;
      }

      state.out << ", ";

      if (inst->rw.rel == SQ_RELATIVE) {
         state.out << "R[AL + " << inst->rw.id + "]";
      } else {
         state.out << "R" << inst->rw.id;
      }

      state.out
         << '.'
         << disassembleDestMask(inst->srcSel[0])
         << disassembleDestMask(inst->srcSel[1])
         << disassembleDestMask(inst->srcSel[2])
         << disassembleDestMask(inst->srcSel[3]);

      inst->rw.rel;
   } else {
      // TODO: Disassemble export MEM_*
      state.out << " MEM_UNKNOWN_FORMAT";
   }

   if (inst->burstCount) {
      state.out << " BURSTCNT(" << inst->burstCount << ")";
   }

   if (!inst->barrier) {
      state.out << " NO_BARRIER";
   }

   if (inst->wholeQuadMode) {
      state.out << " WHOLE_QUAD";
   }

   if (inst->validPixelMode) {
      state.out << " VALID_PIX";
   }

   state.out << "\n";
   return true;
}

} // namespace disassembler

} // namespace latte
