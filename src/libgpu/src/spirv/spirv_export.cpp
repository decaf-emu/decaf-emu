#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

using namespace latte;

static inline VarRefType
getPixelVarType(PixelOutputType pixelType)
{
   switch (pixelType) {
   case PixelOutputType::FLOAT:
      return VarRefType::FLOAT;
   case PixelOutputType::SINT:
      return VarRefType::INT;
   case PixelOutputType::UINT:
      return VarRefType::UINT;
   }

   decaf_abort("Unexpected color output type")
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

      bool skipWrite = false;

      if (mType == ShaderParser::Type::Pixel) {
         // Update the export value type based on the color output format
         if (exportRef.output.type == ExportRef::Type::Pixel ||
             exportRef.output.type == ExportRef::Type::PixelWithFog) {
            auto pixelFormat = mPixelOutType[exportRef.output.arrayBase];
            if (pixelFormat == PixelOutputType::FLOAT) {
               // We are already in the right format, nothing to do here...
            } else if (pixelFormat == PixelOutputType::SINT) {
               sourceVal = mSpv->createUnaryOp(spv::OpBitcast, mSpv->int4Type(), sourceVal);
            } else if (pixelFormat == PixelOutputType::UINT) {
               sourceVal = mSpv->createUnaryOp(spv::OpBitcast, mSpv->uint4Type(), sourceVal);
            }
         }

         // Apply the appropriate masking.
         if (exportRef.output.type == ExportRef::Type::Pixel ||
             exportRef.output.type == ExportRef::Type::PixelWithFog) {

            // Calculate that the number of exports we expect matchs our number
            // of enabled rendertargets, or the search below will fail.
            auto numExports = mSqPgmExportsPs.EXPORT_MODE() >> 1;
            auto rtExports = 0;
            rtExports += mCbShaderControl.RT0_ENABLE() ? 1 : 0;
            rtExports += mCbShaderControl.RT1_ENABLE() ? 1 : 0;
            rtExports += mCbShaderControl.RT2_ENABLE() ? 1 : 0;
            rtExports += mCbShaderControl.RT3_ENABLE() ? 1 : 0;
            rtExports += mCbShaderControl.RT4_ENABLE() ? 1 : 0;
            rtExports += mCbShaderControl.RT5_ENABLE() ? 1 : 0;
            rtExports += mCbShaderControl.RT6_ENABLE() ? 1 : 0;
            rtExports += mCbShaderControl.RT7_ENABLE() ? 1 : 0;
            decaf_check(rtExports == numExports);

            // Skip over render targets which are not being written.
            do {
               auto rtEnabled = !!((mCbShaderControl.value >> exportRef.output.arrayBase) & 0x1);
               if (rtEnabled) {
                  break;
               }

               exportRef.output.arrayBase++;
            } while (exportRef.output.arrayBase < 8);
            decaf_check(exportRef.output.arrayBase < 8);

            auto compMask = (mCbShaderMask.value >> (exportRef.output.arrayBase * 4)) & 0xf;

            spv::Id maskXVal, maskYVal, maskZVal, maskWVal;
            auto sourceValType = mSpv->getTypeId(sourceVal);
            if (sourceValType == mSpv->float4Type()) {
               maskXVal = mSpv->makeFloatConstant(0.0f);
               maskYVal = mSpv->makeFloatConstant(0.0f);
               maskZVal = mSpv->makeFloatConstant(0.0f);
               maskWVal = mSpv->makeFloatConstant(1.0f);
            } else if (sourceValType == mSpv->int4Type()) {
               maskXVal = mSpv->makeIntConstant(0);
               maskYVal = mSpv->makeIntConstant(0);
               maskZVal = mSpv->makeIntConstant(0);
               maskWVal = mSpv->makeIntConstant(1);
            } else if (sourceValType == mSpv->uint4Type()) {
               maskXVal = mSpv->makeUintConstant(0);
               maskYVal = mSpv->makeUintConstant(0);
               maskZVal = mSpv->makeUintConstant(0);
               maskWVal = mSpv->makeUintConstant(1);
            } else {
               decaf_abort("Unexpected texture output format in component masking.");
            }

            if (!(compMask & 1)) {
               sourceVal = mSpv->createOp(spv::OpCompositeInsert, sourceValType, { maskXVal, sourceVal, 0 });
            }
            if (!(compMask & 2)) {
               sourceVal = mSpv->createOp(spv::OpCompositeInsert, sourceValType, { maskYVal, sourceVal, 1 });
            }
            if (!(compMask & 4)) {
               sourceVal = mSpv->createOp(spv::OpCompositeInsert, sourceValType, { maskZVal, sourceVal, 2 });
            }
            if (!(compMask & 8)) {
               sourceVal = mSpv->createOp(spv::OpCompositeInsert, sourceValType, { maskWVal, sourceVal, 3 });
            }
         }

         if (exportRef.output.type == ExportRef::Type::ComputedZ) {
            if (!mDbShaderControl.Z_EXPORT_ENABLE()) {
               // The shader exported a Z, but its not enabled.  Lets skip it.
               skipWrite = true;
            }
         }
      }

      if (mType == ShaderParser::Type::Vertex || mType == ShaderParser::Type::DataCache) {
         // Check if this is a position output special case.
         if (exportRef.output.type == ExportRef::Type::Position) {
            if (exportRef.output.arrayBase == 0) {
               // POS_0 is just a standard position output.
            } else {
               auto pa_cl_vs_out_cntl = mPaClVsOutCntl;

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

               // We have to skip the write, since its already done.
               skipWrite = true;
            }
         }
      }

      // Write the exported data
      if (!skipWrite) {
         mSpv->writeExportRef(exportRef, sourceVal);
      }

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

void Transpiler::translateGenericStream(const ControlFlowInst &cf, int streamIdx)
{
   decaf_check(streamIdx < 4);

   // Find the right stride for this particular streamout.
   auto streamOutStride = mStreamOutStride[streamIdx];

   GprRef srcGpr;
   srcGpr = makeGprRef(cf.exp.word0.RW_GPR(), cf.exp.word0.RW_REL(), SQ_INDEX_MODE::LOOP);

   ExportMaskRef exportRef;
   exportRef.output = makeStreamExportRef(cf.exp.word0.TYPE(),
                                          cf.exp.word0.INDEX_GPR(),
                                          streamIdx,
                                          streamOutStride,
                                          cf.exp.word0.ARRAY_BASE(),
                                          cf.exp.buf.ARRAY_SIZE() + 1,
                                          cf.exp.word0.ELEM_SIZE() + 1);
   exportRef.mask[SQ_CHAN::X] = (cf.exp.buf.COMP_MASK() & (1 << 0)) ? latte::SQ_SEL::SEL_X : latte::SQ_SEL::SEL_MASK;
   exportRef.mask[SQ_CHAN::Y] = (cf.exp.buf.COMP_MASK() & (1 << 1)) ? latte::SQ_SEL::SEL_Y : latte::SQ_SEL::SEL_MASK;
   exportRef.mask[SQ_CHAN::Z] = (cf.exp.buf.COMP_MASK() & (1 << 2)) ? latte::SQ_SEL::SEL_Z : latte::SQ_SEL::SEL_MASK;
   exportRef.mask[SQ_CHAN::W] = (cf.exp.buf.COMP_MASK() & (1 << 3)) ? latte::SQ_SEL::SEL_W : latte::SQ_SEL::SEL_MASK;

   if (isSwizzleFullyMasked(exportRef.mask)) {
      // We should just skip fully masked swizzles.
      return;
   }

   auto exportCount = cf.exp.word1.BURST_COUNT() + 1;
   for (auto i = 0u; i < exportCount; ++i) {
      // Read the source GPR
      auto sourcePtr = mSpv->getGprRef(srcGpr);
      auto sourceVal = mSpv->createLoad(sourcePtr);

      // Write the exported data
      mSpv->writeExportRef(exportRef, sourceVal);

      // Increase the indexing for each export
      srcGpr.next();
      exportRef.output.next();
   }
}

void Transpiler::translateCf_MEM_STREAM0(const ControlFlowInst &cf)
{
   translateGenericStream(cf, 0);
}

void Transpiler::translateCf_MEM_STREAM1(const ControlFlowInst &cf)
{
   translateGenericStream(cf, 1);
}

void Transpiler::translateCf_MEM_STREAM2(const ControlFlowInst &cf)
{
   translateGenericStream(cf, 2);
}

void Transpiler::translateCf_MEM_STREAM3(const ControlFlowInst &cf)
{
   translateGenericStream(cf, 3);
}

void Transpiler::translateCf_MEM_RING(const ControlFlowInst &cf)
{
   GprRef srcGpr;
   srcGpr = makeGprRef(cf.exp.word0.RW_GPR(), cf.exp.word0.RW_REL(), SQ_INDEX_MODE::LOOP);

   ExportMaskRef exportRef;
   if (mType == ShaderParser::Type::Vertex) {
      exportRef.output = makeVsGsRingExportRef(cf.exp.word0.TYPE(),
                                               cf.exp.word0.INDEX_GPR(),
                                               cf.exp.word0.ARRAY_BASE(),
                                               cf.exp.buf.ARRAY_SIZE() + 1,
                                               cf.exp.word0.ELEM_SIZE() + 1);
   } else if (mType == ShaderParser::Type::Geometry) {
      exportRef.output = makeGsDcRingExportRef(cf.exp.word0.TYPE(),
                                               cf.exp.word0.INDEX_GPR(),
                                               cf.exp.word0.ARRAY_BASE(),
                                               cf.exp.buf.ARRAY_SIZE() + 1,
                                               cf.exp.word0.ELEM_SIZE() + 1);
   } else {
      decaf_abort("Unexpected shader type for MEM_RING")
   }
   exportRef.mask[SQ_CHAN::X] = (cf.exp.buf.COMP_MASK() & (1 << 0)) ? latte::SQ_SEL::SEL_X : latte::SQ_SEL::SEL_MASK;
   exportRef.mask[SQ_CHAN::Y] = (cf.exp.buf.COMP_MASK() & (1 << 1)) ? latte::SQ_SEL::SEL_Y : latte::SQ_SEL::SEL_MASK;
   exportRef.mask[SQ_CHAN::Z] = (cf.exp.buf.COMP_MASK() & (1 << 2)) ? latte::SQ_SEL::SEL_Z : latte::SQ_SEL::SEL_MASK;
   exportRef.mask[SQ_CHAN::W] = (cf.exp.buf.COMP_MASK() & (1 << 3)) ? latte::SQ_SEL::SEL_W : latte::SQ_SEL::SEL_MASK;

   if (isSwizzleFullyMasked(exportRef.mask)) {
      // We should just skip fully masked swizzles.
      return;
   }

   auto exportCount = cf.exp.word1.BURST_COUNT() + 1;
   for (auto i = 0u; i < exportCount; ++i) {
      // Read the source GPR
      auto sourcePtr = mSpv->getGprRef(srcGpr);
      auto sourceVal = mSpv->createLoad(sourcePtr);

      // Write the exported data
      mSpv->writeExportRef(exportRef, sourceVal);

      // Increase the indexing for each export
      srcGpr.next();
      exportRef.output.next();
   }
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
