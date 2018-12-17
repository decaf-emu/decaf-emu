#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"
#include "latte/latte_formats.h"

namespace spirv
{

void Transpiler::translateVtx_FETCH(const ControlFlowInst &cf, const VertexFetchInst &inst)
{
   // MEGA fetches are an optimization on the GPU cache.  There is
   // no need to do any particular work during translation.
   //inst.word2.MEGA_FETCH()
   //inst.word0.MEGA_FETCH_COUNT()

   // Let's only support a very expected set of values
   decaf_check(inst.word0.FETCH_TYPE() == SQ_VTX_FETCH_TYPE::NO_INDEX_OFFSET);
   decaf_check(inst.word1.USE_CONST_FIELDS() == 1);

   // Grab the source register information
   GprSelRef srcGpr;
   srcGpr.gpr = makeGprRef(inst.word0.SRC_GPR(), inst.word0.SRC_REL(), SQ_INDEX_MODE::LOOP);
   srcGpr.sel = inst.word0.SRC_SEL_X();

   // Grab the destination register information
   GprMaskRef destGpr;
   destGpr.gpr = makeGprRef(inst.gpr.DST_GPR(), inst.gpr.DST_REL(), SQ_INDEX_MODE::LOOP);
   destGpr.mask[SQ_CHAN::X] = inst.word1.DST_SEL_X();
   destGpr.mask[SQ_CHAN::Y] = inst.word1.DST_SEL_Y();
   destGpr.mask[SQ_CHAN::Z] = inst.word1.DST_SEL_Z();
   destGpr.mask[SQ_CHAN::W] = inst.word1.DST_SEL_W();

   // Intercept a GSIN resource fetch, and process it specially.
   if (mType == ShaderParser::Type::Geometry) {
      auto id = inst.word0.BUFFER_ID() + SQ_RES_OFFSET::GS_TEX_RESOURCE_0;
      if (id == SQ_RES_OFFSET::GS_GSIN_RESOURCE) {

         // We only support vec4 ring buffer items
         decaf_check(inst.word2.OFFSET() % 16 == 0);
         auto ringOffset = inst.word2.OFFSET() / 16;

         auto inputVar = mSpv->inputRingVar(ringOffset);

         // We check to make sure everything makes sense...
         decaf_check(srcGpr.gpr.indexMode == GprIndexMode::None);

         spv::Id vertIdxVal;
         if (srcGpr.gpr.number == 0 && srcGpr.sel == latte::SQ_SEL::SEL_X) {
            vertIdxVal = mSpv->makeIntConstant(0);
         } else if (srcGpr.gpr.number == 0 && srcGpr.sel == latte::SQ_SEL::SEL_Y) {
            vertIdxVal = mSpv->makeIntConstant(1);
         } else if (srcGpr.gpr.number == 0 && srcGpr.sel == latte::SQ_SEL::SEL_W) {
            vertIdxVal = mSpv->makeIntConstant(2);
         } else if (srcGpr.gpr.number == 1 && srcGpr.sel == latte::SQ_SEL::SEL_X) {
            vertIdxVal = mSpv->makeIntConstant(3);
         } else if (srcGpr.gpr.number == 1 && srcGpr.sel == latte::SQ_SEL::SEL_Y) {
            vertIdxVal = mSpv->makeIntConstant(4);
         } else if (srcGpr.gpr.number == 1 && srcGpr.sel == latte::SQ_SEL::SEL_W) {
            vertIdxVal = mSpv->makeIntConstant(5);
         } else {
            decaf_abort("Unexpected vertex index selection")
         }

         auto outputPtr = mSpv->createAccessChain(spv::StorageClassInput, inputVar, { vertIdxVal });
         auto outputVal = mSpv->createLoad(outputPtr);

         auto gprRef = mSpv->getGprRef(destGpr.gpr);

         auto destVal = spv::NoResult;
         if (!latte::isSwizzleFullyUnmasked(destGpr.mask)) {
            destVal = mSpv->createLoad(gprRef);
         }

         auto maskedVal = mSpv->applySelMask(destVal, outputVal, destGpr.mask);

         mSpv->createStore(maskedVal, gprRef);

         return;
      }
   } else if (mType == ShaderParser::Type::DataCache) {
      auto id = inst.word0.BUFFER_ID() + SQ_RES_OFFSET::GS_TEX_RESOURCE_0;
      if (id == SQ_RES_OFFSET::GS_GSIN_RESOURCE) {
         auto ringOffsetVal = mSpv->createLoad(mSpv->ringOffsetVar());

         // We check to make sure everything makes sense...
         decaf_check(srcGpr.gpr.number == 0);
         decaf_check(srcGpr.gpr.indexMode == GprIndexMode::None);
         decaf_check(srcGpr.sel == latte::SQ_SEL::SEL_X);

         // Note that this is blocked out above, so this code may not be correct...
         decaf_check(inst.word2.OFFSET() % 16 == 0);
         auto itemOffsetVal = mSpv->makeUintConstant(inst.word2.OFFSET() / 16);

         auto realOffset = mSpv->createBinOp(spv::OpIAdd, mSpv->uintType(), ringOffsetVal, itemOffsetVal);

         auto outputPtr = mSpv->createAccessChain(spv::StorageClassPrivate, mSpv->ringVar(), { realOffset });
         auto outputVal = mSpv->createLoad(outputPtr);

         auto gprRef = mSpv->getGprRef(destGpr.gpr);

         // TODO: This logic is duplicated a bunch of places, we should converge
         // this into a centralized function in mSpv...
         auto destVal = spv::NoResult;
         if (!latte::isSwizzleFullyUnmasked(destGpr.mask)) {
            destVal = mSpv->createLoad(gprRef);
         }

         auto maskedVal = mSpv->applySelMask(destVal, outputVal, destGpr.mask);

         mSpv->createStore(maskedVal, gprRef);

         return;
      }
   }

   decaf_check(inst.word2.OFFSET() == 0);

   uint32_t cbufferIdx;
   if (mType == ShaderParser::Type::Vertex) {
      auto id = inst.word0.BUFFER_ID() + SQ_RES_OFFSET::VS_TEX_RESOURCE_0;

      if (id >= SQ_RES_OFFSET::VS_BUF_RESOURCE_0 && id <= SQ_RES_OFFSET::VS_BUF_RESOURCE_0 + 16) {
         cbufferIdx = id - SQ_RES_OFFSET::VS_BUF_RESOURCE_0;

      } else {
         decaf_abort("Unsupported vertex shader VTX_FETCH vertex resource");
      }
   } else if (mType == ShaderParser::Type::Geometry) {
      auto id = inst.word0.BUFFER_ID() + SQ_RES_OFFSET::GS_TEX_RESOURCE_0;

      if (id >= SQ_RES_OFFSET::GS_BUF_RESOURCE_0 && id <= SQ_RES_OFFSET::GS_BUF_RESOURCE_0 + 16) {
         cbufferIdx = id - SQ_RES_OFFSET::GS_BUF_RESOURCE_0;
      } else {
         decaf_abort("Unsupported vertex shader VTX_FETCH geometry resource");
      }
   } else if (mType == ShaderParser::Type::Pixel) {
      auto id = inst.word0.BUFFER_ID() + SQ_RES_OFFSET::PS_TEX_RESOURCE_0;

      if (id >= SQ_RES_OFFSET::PS_BUF_RESOURCE_0 && id <= SQ_RES_OFFSET::PS_BUF_RESOURCE_0 + 16) {
         cbufferIdx = id - SQ_RES_OFFSET::PS_BUF_RESOURCE_0;
      } else {
         decaf_abort("Unsupported vertex shader VTX_FETCH pixel resource");
      }
   } else {
      decaf_abort("Unsupported shader type for VTX_FETCH");
   }

   auto indexValFloat = mSpv->readGprSelRef(srcGpr);
   auto indexVal = mSpv->createUnaryOp(spv::OpBitcast, mSpv->uintType(), indexValFloat);

   auto cbufferVar = mSpv->cbufferVar(cbufferIdx);

   // TODO: Should probably move this into SPV
   auto zeroConst = mSpv->makeUintConstant(0);
   auto cbufferPtr = mSpv->createAccessChain(spv::StorageClassUniform, cbufferVar, { zeroConst, indexVal });
   auto outputVal = mSpv->createLoad(cbufferPtr);

   auto gprRef = mSpv->getGprRef(destGpr.gpr);

   // TODO: This logic is duplicated a bunch of places, we should converge
   // this into a centralized function in mSpv...
   auto destVal = spv::NoResult;
   if (!latte::isSwizzleFullyUnmasked(destGpr.mask)) {
      destVal = mSpv->createLoad(gprRef);
   }

   auto maskedVal = mSpv->applySelMask(destVal, outputVal, destGpr.mask);

   mSpv->createStore(maskedVal, gprRef);
}

int findVtxSemanticGpr(const std::array<latte::SQ_VTX_SEMANTIC_N, 32>& vtxSemantics, uint8_t semanticId)
{
   for (auto i = 0; i < 32; ++i) {
      auto foundSemanticId = vtxSemantics[i].SEMANTIC_ID();
      if (semanticId == foundSemanticId) {
         return i;
      }
   }
   return -1;
}

void Transpiler::translateVtx_SEMANTIC(const ControlFlowInst &cf, const VertexFetchInst &inst)
{
   // Because this instruction has an SRC register to determine where to source
   //  the indexing data from, but we need a constant, we assume that it will always
   //  use R0, and that it will contain what it originally starts with during shader
   //  startup.  In order to ensure this is the case, we only allow SEMANTIC inst's
   //  to execute within a Fetch Shader for now, and we also ensure that the fetch
   //  shader is always invoked as the first instruction of a vertex shader.  We
   //  hope that this never needs to be fixed, otherwise things are going to get
   //  EXTREMELY complicated (need to do register expression propagation).
   decaf_check(mType == ShaderParser::Type::Fetch);

   // We do not support fetch constant fields as I don't even
   //  know what they are at the moment!
   decaf_check(!inst.word1.USE_CONST_FIELDS());

   // More stuff I have no clue about...
   decaf_check(!inst.word1.SRF_MODE_ALL());

   // We know what this one does, but I don't know how to implement it,
   //  and I do not think it will be needed right now...
   decaf_check(!inst.word2.CONST_BUF_NO_STRIDE());

   // We use the DATA_FORMAT to determine the fetch sizing.  This allows us
   //  to more easily handle the splitting of XYZW groups below.
   //inst.word2.MEGA_FETCH();
   //inst.word0.MEGA_FETCH_COUNT();

   GprSelRef srcGpr;
   srcGpr.gpr = makeGprRef(inst.word0.SRC_GPR(), inst.word0.SRC_REL(), SQ_INDEX_MODE::LOOP);
   srcGpr.sel = inst.word0.SRC_SEL_X();

   // We do not support indexing for fetch from buffers...
   decaf_check(srcGpr.gpr.indexMode == GprIndexMode::None);

   // Try and locate a matching semantic from the semantic table
   auto semanticGprIdx = findVtxSemanticGpr(mSqVtxSemantics, inst.sem.SEMANTIC_ID());
   if (semanticGprIdx < 0) {
      // We didn't find anythign in the semantic table, we can simply
      // ignore this instruction in that case.
      return;
   }

   GprMaskRef destGpr;
   destGpr.gpr.indexMode = GprIndexMode::None;
   destGpr.gpr.number = static_cast<uint32_t>(semanticGprIdx + 1);
   destGpr.mask[SQ_CHAN::X] = inst.word1.DST_SEL_X();
   destGpr.mask[SQ_CHAN::Y] = inst.word1.DST_SEL_Y();
   destGpr.mask[SQ_CHAN::Z] = inst.word1.DST_SEL_Z();
   destGpr.mask[SQ_CHAN::W] = inst.word1.DST_SEL_W();

   if (destGpr.gpr.number == 0xffffffff) {
      // This is not semantically mapped, so we can actually skip it entirely!
      return;
   }

   auto dataFormat = inst.word1.DATA_FORMAT();
   auto fmtMeta = latte::getDataFormatMeta(dataFormat);

   // We currently only support vertex fetches inside a fetch shader...
   decaf_check(mType == ShaderParser::Type::Fetch);

   // Figure out which attribute buffer this is referencing
   auto bufferId = inst.word0.BUFFER_ID();
   auto attribBase = latte::SQ_RES_OFFSET::VS_ATTRIB_RESOURCE_0 - latte::SQ_RES_OFFSET::VS_TEX_RESOURCE_0;
   decaf_check(attribBase <= bufferId && bufferId <= attribBase + 16);

   auto attribBufferId = bufferId - attribBase;
   auto bufferOffset = inst.word2.OFFSET();

   AttribBuffer::IndexMode indexMode;
   AttribBuffer::DivisorMode divisorMode;

   if (inst.word0.FETCH_TYPE() == SQ_VTX_FETCH_TYPE::VERTEX_DATA) {
      indexMode = AttribBuffer::IndexMode::PerVertex;
      divisorMode = AttribBuffer::DivisorMode::CONST_1;
   } else if (inst.word0.FETCH_TYPE() == SQ_VTX_FETCH_TYPE::INSTANCE_DATA) {
      indexMode = AttribBuffer::IndexMode::PerInstance;

      if (srcGpr.sel == SQ_SEL::SEL_Y) {
         divisorMode = AttribBuffer::DivisorMode::REGISTER_0;
      } else if (srcGpr.sel == SQ_SEL::SEL_Z) {
         divisorMode = AttribBuffer::DivisorMode::REGISTER_1;
      } else if (srcGpr.sel == SQ_SEL::SEL_W) {
         divisorMode = AttribBuffer::DivisorMode::CONST_1;
      } else {
         decaf_abort("Unexpected vertex fetch divisor selector");
      }
   } else {
      decaf_abort("Unexpected vertex fetch type");
   }

   // Set up our buffer information
   auto& inputBuffer = mAttribBuffers[attribBufferId];
   if (inputBuffer.isUsed) {
      // If its already in use, confirm matching indexing
      decaf_check(inputBuffer.indexMode == indexMode);
      decaf_check(inputBuffer.divisorMode == divisorMode);
   } else {
      // If its not in use, lets set this up ourselves.
      inputBuffer = AttribBuffer { true, indexMode, divisorMode };
   }

   // Set up our attribute information
   AttribElem inputAttrib;
   inputAttrib.bufferIndex = attribBufferId;
   inputAttrib.offset = bufferOffset;
   inputAttrib.elemWidth = fmtMeta.inputWidth;
   inputAttrib.elemCount = fmtMeta.inputCount;
   mAttribElems.push_back(inputAttrib);
   auto inputId = mAttribElems.size() - 1;

   auto swapMode = inst.word2.ENDIAN_SWAP();
   auto formatComp = inst.word1.FORMAT_COMP_ALL();
   auto numFormat = inst.word1.NUM_FORMAT_ALL();

   spv::Id sourceElemType = mSpv->uintType();

   // Get a vector of the source element type
   auto sourceType = mSpv->vecType(sourceElemType, fmtMeta.inputCount);

   // Create the input variable
   auto sourceVar = mSpv->inputAttribVar(static_cast<uint32_t>(inputId), sourceType);

   // Load the input data
   auto source = mSpv->createLoad(sourceVar);

   // Perform byte swapping on the input data, where appropriate
   if (swapMode == SQ_ENDIAN::SWAP_8IN16) {
      decaf_check(fmtMeta.inputWidth == 16 || fmtMeta.inputWidth == 32);
      source = mSpv->bswap8in16(source);
   } else if (swapMode == SQ_ENDIAN::SWAP_8IN32) {
      decaf_check(fmtMeta.inputWidth == 32);
      source = mSpv->bswap8in32(source);
   } else if (swapMode != SQ_ENDIAN::NONE) {
      decaf_abort("Encountered unexpected endian swap mode");
   }

   // Store each element for operating on independantly...
   std::array<spv::Id, 4> inputElems = { spv::NoResult, spv::NoResult, spv::NoResult, spv::NoResult };
   if (fmtMeta.inputCount > 1) {
      for (auto i = 0u; i < fmtMeta.inputCount; ++i) {
         inputElems[i] = mSpv->createOp(spv::OpCompositeExtract, sourceElemType, { source, i });
      }
   } else {
      for (auto i = 0u; i < fmtMeta.inputCount; ++i) {
         inputElems[i] = source;
      }
   }

   // Upcast each element to a uint for bitshift extraction
   if (sourceElemType != mSpv->uintType()) {
      for (auto i = 0u; i < fmtMeta.inputCount; ++i) {
         inputElems[i] = mSpv->createUnaryOp(spv::OpUConvert, mSpv->uintType(), inputElems[i]);
      }
   }

   // Calculate the number of resulting elements based on the lengths
   int outputElemCount = 0;
   for (auto i = 0u; i < 4u; ++i) {
      if (fmtMeta.elems[i].length > 0) {
         outputElemCount = i + 1;
      }
   }

   // Extract the appropriate bits if needed...
   std::array<spv::Id, 4> elems = { spv::NoResult, spv::NoResult, spv::NoResult, spv::NoResult };
   for (auto i = 0; i < outputElemCount; ++i) {
      auto &elem = fmtMeta.elems[i];

      // If the element width matches perfectly, we can just use it directly
      if (elem.length == fmtMeta.inputWidth) {
         elems[i] = inputElems[elem.index];
         continue;
      }

      auto startConst = mSpv->makeIntConstant(elem.start);
      auto lengthConst = mSpv->makeIntConstant(elem.length);
      elems[i] = mSpv->createTriOp(spv::OpBitFieldUExtract, mSpv->uintType(), inputElems[elem.index], startConst, lengthConst);
   }

   for (auto i = 0; i < outputElemCount; ++i) {
      auto &elem = fmtMeta.elems[i];
      auto fieldMax = static_cast<uint64_t>(1u) << elem.length;

      if (fmtMeta.type == DataFormatMetaType::FLOAT) {
         if (elem.length == 16) {
            // Bitcast the data to a float from half-float data
            // elem = unpackHalf2x16(elem).x
            auto unpackedFloat2 = mSpv->createBuiltinCall(mSpv->float2Type(), mSpv->glslStd450(), GLSLstd450::GLSLstd450UnpackHalf2x16, { elems[i] });
            elems[i] = mSpv->createOp(spv::Op::OpCompositeExtract, mSpv->floatType(), { unpackedFloat2, 0 });
         } else if (elem.length == 32) {
            // Bitcast the data to a float
            // elem = *(float*)&elem
            elems[i] = mSpv->createUnaryOp(spv::Op::OpBitcast, mSpv->floatType(), elems[i]);
         } else if (elem.length == 10) {
            elems[i] = mSpv->unpackFloat10(elems[i]);
         } else if (elem.length == 11) {
            elems[i] = mSpv->unpackFloat11(elems[i]);
         } else {
            decaf_abort("Unexpected float data width");
         }
      } else {
         if (formatComp == latte::SQ_FORMAT_COMP::SIGNED && elem.length > 2) {
            // Perform sign-extension and conversion to a signed integer
            if (elem.length == 32) {
               // If its 32 bits, we don't need to perform sign extension, just bitcast it
               // elem = *(int*)&elem
               elems[i] = mSpv->createUnaryOp(spv::Op::OpBitcast, mSpv->intType(), elems[i]);
            } else {
               // If its less than 32 bits, we use bitfield extraction to sign extend
               auto offsetConst = mSpv->makeIntConstant(0);
               auto lengthConst = mSpv->makeIntConstant(elem.length);
               auto signedElem = mSpv->createUnaryOp(spv::OpBitcast, mSpv->intType(), elems[i]);
               elems[i] = mSpv->createTriOp(spv::Op::OpBitFieldSExtract, mSpv->intType(), signedElem, offsetConst, lengthConst);
            }
         } else {
            // We are already in UINT format as we needed
         }
      }

      if (numFormat == latte::SQ_NUM_FORMAT::NORM) {
         decaf_check(fmtMeta.type == DataFormatMetaType::UINT);

         auto fieldMask = fieldMax - 1;

         if (formatComp == latte::SQ_FORMAT_COMP::SIGNED && elem.length > 2) {
            // Type must already be a signed type from above
            decaf_check(mSpv->getTypeId(elems[i]) == mSpv->intType());

            // elem = clamp((float)(elem) / float(FIELD_MASK/2), -1.0f, 1.0f)
            auto normMaxConst = mSpv->makeFloatConstant(float(fieldMask / 2));
            auto normNegConst = mSpv->makeFloatConstant(-1.0f);
            auto normPosConst = mSpv->makeFloatConstant(+1.0f);
            auto floatElem = mSpv->createUnaryOp(spv::OpConvertSToF, mSpv->floatType(), elems[i]);
            auto normElem = mSpv->createBinOp(spv::OpFDiv, mSpv->floatType(), floatElem, normMaxConst);
            elems[i] = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FClamp,
                                               { normElem, normNegConst, normPosConst });
         } else {
            // Type must already be a unsigned type from above
            decaf_check(mSpv->getTypeId(elems[i]) == mSpv->uintType());

            // elem = float(elem) / float(FIELD_MASK)
            auto normMaxConst = mSpv->makeFloatConstant(float(fieldMask));
            auto floatElem = mSpv->createUnaryOp(spv::OpConvertSToF, mSpv->floatType(), elems[i]);
            elems[i] = mSpv->createBinOp(spv::OpFDiv, mSpv->floatType(), floatElem, normMaxConst);
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT::INT) {
         auto origTypeId = mSpv->getTypeId(elems[i]);

         if (formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
            // elem = int(elem)
            if (origTypeId == mSpv->floatType()) {
               elems[i] = mSpv->createUnaryOp(spv::Op::OpConvertFToS, mSpv->intType(), elems[i]);
            } else if (origTypeId == mSpv->intType()) {
               // We are already an int type, no need to convert
            } else if (origTypeId == mSpv->uintType()) {
               elems[i] = mSpv->createUnaryOp(spv::OpBitcast, mSpv->intType(), elems[i]);
            } else {
               decaf_abort("Unexpected format conversion type.");
            }
         } else {
            // elem = uint(elem)
            if (origTypeId == mSpv->floatType()) {
               elems[i] = mSpv->createUnaryOp(spv::Op::OpConvertFToU, mSpv->uintType(), elems[i]);
            } else if (origTypeId == mSpv->intType()) {
               elems[i] = mSpv->createUnaryOp(spv::OpBitcast, mSpv->uintType(), elems[i]);
            } else if (origTypeId == mSpv->uintType()) {
               // We are already a uint type, no need to convert
            } else {
               decaf_abort("Unexpected format conversion type.");
            }
         }
      } else if (numFormat == latte::SQ_NUM_FORMAT::SCALED) {
         // formatComp ignored as SIGNED/UNSIGNED makes no sense at this stage

         auto origTypeId = mSpv->getTypeId(elems[i]);

         // elem = float(elem)
         if (origTypeId == mSpv->floatType()) {
            // We are already a float type, no need to convert
         } else if (origTypeId == mSpv->intType()) {
            elems[i] = mSpv->createUnaryOp(spv::Op::OpConvertSToF, mSpv->floatType(), elems[i]);
         } else if (origTypeId == mSpv->uintType()) {
            elems[i] = mSpv->createUnaryOp(spv::Op::OpConvertUToF, mSpv->floatType(), elems[i]);
         } else {
            decaf_abort("Unexpected format conversion type.");
         }
      } else {
         decaf_abort("Unexpected vertex fetch numFormat");
      }
   }

   // Figure out what format of elements we have as output
   auto elemsType = mSpv->getTypeId(elems[0]);
   for (auto i = 1; i < outputElemCount; ++i) {
      decaf_check(mSpv->getTypeId(elems[i]) == elemsType);
   }

   // Fill remaining values with defaults
   for (auto i = outputElemCount; i < 4; ++i) {
      if (elemsType == mSpv->floatType()) {
         if (i != 3) {
            elems[i] = mSpv->makeFloatConstant(0.0f);
         } else {
            elems[i] = mSpv->makeFloatConstant(1.0f);
         }
      } else if (elemsType == mSpv->intType()) {
         if (i != 3) {
            elems[i] = mSpv->makeIntConstant(0);
         } else {
            elems[i] = mSpv->makeIntConstant(1);
         }
      } else if (elemsType == mSpv->uintType()) {
         if (i != 3) {
            elems[i] = mSpv->makeUintConstant(0);
         } else {
            elems[i] = mSpv->makeUintConstant(1);
         }
      } else {
         decaf_abort("Unexpected element format output in fetch")
      }
   }

   auto outputType = mSpv->vecType(elemsType, 4);
   auto outputVal = mSpv->createOp(spv::Op::OpCompositeConstruct, outputType,
                                   { elems[0], elems[1], elems[2], elems[3] });

   mSpv->writeGprMaskRef(destGpr, outputVal);
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
