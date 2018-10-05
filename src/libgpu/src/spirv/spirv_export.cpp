#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

using namespace latte;

VarRefType getPixelVarType(ColorOutputType pixelType)
{
   switch (pixelType) {
   case ColorOutputType::FLOAT:
      return VarRefType::FLOAT;
   case ColorOutputType::SINT:
      return VarRefType::INT;
   case ColorOutputType::UINT:
      return VarRefType::UINT;
   }

   decaf_abort("Unexpected color output type")
}

void Transpiler::translateGenericExport(const ControlFlowInst &cf)
{
   GprMaskRef srcGpr;
   srcGpr.gpr = makeGprRef(cf.exp.word0.RW_GPR(), cf.exp.word0.RW_REL(), SQ_INDEX_MODE::LOOP);
   srcGpr.mask[SQ_CHAN::X] = cf.exp.swiz.SRC_SEL_X();
   srcGpr.mask[SQ_CHAN::Y] = cf.exp.swiz.SRC_SEL_Y();
   srcGpr.mask[SQ_CHAN::Z] = cf.exp.swiz.SRC_SEL_Z();
   srcGpr.mask[SQ_CHAN::W] = cf.exp.swiz.SRC_SEL_W();

   auto exportRef = makeExportRef(cf.exp.word0.TYPE(), cf.exp.word0.ARRAY_BASE());

   if (isSwizzleFullyMasked(srcGpr.mask)) {
      // We should just skip fully masked swizzles.
      return;
   }

   // cf.exp.word0.ELEM_SIZE() is ignored for exports

   auto exportCount = cf.exp.word1.BURST_COUNT() + 1;
   for (auto i = 0u; i < exportCount; ++i) {
      // Read the source GPR
      auto sourcePtr = mSpv->getGprRef(srcGpr.gpr);
      auto sourceVal = mSpv->createLoad(sourcePtr);

      // Update the export value type based on the color output format
      if (exportRef.type == ExportRef::Type::Pixel || exportRef.type == ExportRef::Type::PixelWithFog) {
         exportRef.valueType = getPixelVarType(mPixelOutType[exportRef.index]);
      }

      // Write the exported data
      mSpv->writeExportRef(exportRef, srcGpr.mask, sourceVal);

      // Increase the GPR read number for each export
      srcGpr.gpr.number++;

      // Increase the export output index
      exportRef.index++;
   }
}

void Transpiler::translateCf_EXP(const ControlFlowInst &cf)
{
   translateGenericExport(cf);
}

void Transpiler::translateCf_EXP_DONE(const ControlFlowInst &cf)
{
   translateGenericExport(cf);
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
