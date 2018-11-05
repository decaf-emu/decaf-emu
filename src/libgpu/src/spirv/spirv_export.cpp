#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

using namespace latte;

static inline VarRefType
getPixelVarType(ColorOutputType pixelType)
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

static inline latte::PA_CL_VS_OUT_CNTL
getPaClVsOutCntl(const ShaderDesc *desc)
{
   if (desc->type == ShaderType::Vertex) {
      auto vsDesc = reinterpret_cast<const VertexShaderDesc*>(desc);
      return vsDesc->regs.pa_cl_vs_out_cntl;
   } else {
      decaf_abort("Unexpected shader type for PA_CL_VS_OUT_CNTL fetch")
   }
}

static inline void
calcSpecialVecPositions(latte::PA_CL_VS_OUT_CNTL pa_cl_vs_out_cntl, uint32_t *miscVec, uint32_t *ccdist0Vec, uint32_t *ccdist1Vec)
{
   uint32_t currentPos = 1;

   // TODO: When we encounter these, check the expected ordering.
   decaf_check(!pa_cl_vs_out_cntl.VS_OUT_CCDIST0_VEC_ENA());
   decaf_check(!pa_cl_vs_out_cntl.VS_OUT_CCDIST1_VEC_ENA());

   if (pa_cl_vs_out_cntl.VS_OUT_MISC_VEC_ENA()) {
      if (miscVec) {
         *miscVec = currentPos;
      }
      currentPos++;
   }

   if (pa_cl_vs_out_cntl.VS_OUT_CCDIST0_VEC_ENA()) {
      if (ccdist0Vec) {
         *ccdist0Vec = currentPos;
      }
      currentPos++;
   }

   if (pa_cl_vs_out_cntl.VS_OUT_CCDIST1_VEC_ENA()) {
      if (ccdist1Vec) {
         *ccdist1Vec = currentPos;
      }
      currentPos++;
   }
}

static inline uint32_t
calcMiscVecPos(latte::PA_CL_VS_OUT_CNTL pa_cl_vs_out_cntl)
{
   uint32_t output;
   calcSpecialVecPositions(pa_cl_vs_out_cntl, &output, nullptr, nullptr);
   return output;
}

static inline uint32_t
calcCdist0VecPos(latte::PA_CL_VS_OUT_CNTL pa_cl_vs_out_cntl)
{
   uint32_t output;
   calcSpecialVecPositions(pa_cl_vs_out_cntl, nullptr, &output, nullptr);
   return output;
}

static inline uint32_t
calcCdist1VecPos(latte::PA_CL_VS_OUT_CNTL pa_cl_vs_out_cntl)
{
   uint32_t output;
   calcSpecialVecPositions(pa_cl_vs_out_cntl, nullptr, nullptr, &output);
   return output;
}

void Transpiler::translateGenericExport(const ControlFlowInst &cf)
{
   // cf.exp.word0.ARRAY_SIZE() is ignored for exports
   // cf.exp.word0.ELEM_SIZE() is ignored for exports

   GprRef srcGpr;
   srcGpr = makeGprRef(cf.exp.word0.RW_GPR(), cf.exp.word0.RW_REL(), SQ_INDEX_MODE::LOOP);

   ExportMaskRef exportRef;
   exportRef.output = makeExportRef(cf.exp.word0.TYPE(), cf.exp.word0.ARRAY_BASE());
   exportRef.mask[SQ_CHAN::X] = cf.exp.swiz.SEL_X();
   exportRef.mask[SQ_CHAN::Y] = cf.exp.swiz.SEL_Y();
   exportRef.mask[SQ_CHAN::Z] = cf.exp.swiz.SEL_Z();
   exportRef.mask[SQ_CHAN::W] = cf.exp.swiz.SEL_W();

   if (isSwizzleFullyMasked(exportRef.mask)) {
      // We should just skip fully masked swizzles.
      return;
   }

   auto exportCount = cf.exp.word1.BURST_COUNT() + 1;
   for (auto i = 0u; i < exportCount; ++i) {
      // Read the source GPR
      auto sourcePtr = mSpv->getGprRef(srcGpr);
      auto sourceVal = mSpv->createLoad(sourcePtr);

      if (mType == ShaderParser::Type::Pixel) {
         // Update the export value type based on the color output format
         if (exportRef.output.type == ExportRef::Type::Pixel ||
             exportRef.output.type == ExportRef::Type::PixelWithFog) {
            auto pixelFormat = mPixelOutType[exportRef.output.arrayBase];
            if (pixelFormat == ColorOutputType::FLOAT) {
               // We are already in the right format, nothing to do here...
            } else if (pixelFormat == ColorOutputType::SINT) {
               sourceVal = mSpv->createUnaryOp(spv::OpBitcast, mSpv->int4Type(), sourceVal);
            } else if (pixelFormat == ColorOutputType::UINT) {
               sourceVal = mSpv->createUnaryOp(spv::OpBitcast, mSpv->uint4Type(), sourceVal);
            }
         }
      }

      if (mType == ShaderParser::Type::Vertex) {
         // Check if this is a position output special case.
         if (exportRef.output.type == ExportRef::Type::Position) {
            if (exportRef.output.arrayBase == 0) {
               // POS_0 is just a standard position output.
            } else {
               auto pa_cl_vs_out_cntl = getPaClVsOutCntl(mDesc);

               // Lets check for things we do not support
               decaf_check(!pa_cl_vs_out_cntl.CLIP_DIST_ENA_0());
               decaf_check(!pa_cl_vs_out_cntl.CLIP_DIST_ENA_1());
               decaf_check(!pa_cl_vs_out_cntl.CLIP_DIST_ENA_2());
               decaf_check(!pa_cl_vs_out_cntl.CLIP_DIST_ENA_3());
               decaf_check(!pa_cl_vs_out_cntl.CLIP_DIST_ENA_4());
               decaf_check(!pa_cl_vs_out_cntl.CLIP_DIST_ENA_5());
               decaf_check(!pa_cl_vs_out_cntl.CLIP_DIST_ENA_6());
               decaf_check(!pa_cl_vs_out_cntl.CLIP_DIST_ENA_7());
               decaf_check(!pa_cl_vs_out_cntl.CULL_DIST_ENA_0());
               decaf_check(!pa_cl_vs_out_cntl.CULL_DIST_ENA_1());
               decaf_check(!pa_cl_vs_out_cntl.CULL_DIST_ENA_2());
               decaf_check(!pa_cl_vs_out_cntl.CULL_DIST_ENA_3());
               decaf_check(!pa_cl_vs_out_cntl.CULL_DIST_ENA_4());
               decaf_check(!pa_cl_vs_out_cntl.CULL_DIST_ENA_5());
               decaf_check(!pa_cl_vs_out_cntl.CULL_DIST_ENA_6());
               decaf_check(!pa_cl_vs_out_cntl.CULL_DIST_ENA_7());
               decaf_check(!pa_cl_vs_out_cntl.USE_VTX_POINT_SIZE());
               decaf_check(!pa_cl_vs_out_cntl.USE_VTX_EDGE_FLAG());
               decaf_check(!pa_cl_vs_out_cntl.USE_VTX_VIEWPORT_INDX());
               decaf_check(!pa_cl_vs_out_cntl.USE_VTX_KILL_FLAG());
               decaf_check(!pa_cl_vs_out_cntl.VS_OUT_CCDIST0_VEC_ENA());
               decaf_check(!pa_cl_vs_out_cntl.VS_OUT_CCDIST1_VEC_ENA());
               decaf_check(!pa_cl_vs_out_cntl.USE_VTX_GS_CUT_FLAG());

               // The MISC side-bus seems related to the VS_OUT_MISC_VEC_ENA
               // pa_cl_vs_out_cntl.VS_OUT_MISC_SIDE_BUS_ENA()

               // We have to manually swizzle the value here for use below
               sourceVal = mSpv->applySelMask(spv::NoResult, sourceVal, exportRef.mask);

               if (exportRef.output.arrayBase == calcMiscVecPos(pa_cl_vs_out_cntl)) {
                  if (pa_cl_vs_out_cntl.USE_VTX_RENDER_TARGET_INDX()) {
                     auto sourceZ = mSpv->createOp(spv::OpCompositeExtract, mSpv->floatType(), { sourceVal, 2 });
                     auto sourceZInt = mSpv->createUnaryOp(spv::OpBitcast, mSpv->intType(), sourceZ);
                     mSpv->createStore(sourceZInt, mSpv->layerIdVar());
                  }
               } else if (exportRef.output.arrayBase == calcCdist0VecPos(pa_cl_vs_out_cntl)) {
                  decaf_abort("Unsupported CCDIST0 usage");
               } else if (exportRef.output.arrayBase == calcCdist1VecPos(pa_cl_vs_out_cntl)) {
                  decaf_abort("Unsupported CCDIST1 usage");
               } else {
                  decaf_abort("Unexpected position export index");
               }

               // We have to do a specialized escape here.
               srcGpr.next();
               exportRef.output.next();
               continue;
            }
         }
      }

      // Write the exported data
      mSpv->writeExportRef(exportRef, sourceVal);

      // Increase the indexing for each export
      srcGpr.next();
      exportRef.output.next();
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
