#pragma once
#ifdef DECAF_VULKAN
#include "latte/latte_shaderparser.h"
#include "spirv_spvbuilder.h"

#include <GLSL.std.450.h>

namespace spirv
{

using namespace latte;

enum class ConstantsMode : uint32_t
{
   Unknown,
   Registers,
   Buffers
};

class ShaderSpvBuilder : public SpvBuilder
{
public:
   ShaderSpvBuilder(spv::ExecutionModel execModel)
   {
      setEmitOpLines();
      setSource(spv::SourceLanguage::SourceLanguageUnknown, 0);
      setMemoryModel(spv::AddressingModel::AddressingModelLogical, spv::MemoryModel::MemoryModelGLSL450);
      addCapability(spv::Capability::CapabilityShader);

      auto mainFn = makeEntryPoint("main");
      mFunctions["main"] = mainFn;

      auto entry = addEntryPoint(execModel, mainFn, "main");

      mEntryPoint = entry;
   }

   void setDescriptorSetIdx(int descriptorSetIdx)
   {
      mDescriptorSetIndex = descriptorSetIdx;
   }

   void pushStack()
   {
      // stackIndexVal = *stackIndexVar
      auto stackIdxVal = createLoad(stackIndexVar());
      addName(stackIdxVal, "stackIdx");

      // state = *stateVar
      auto stateVal = createLoad(stateVar());
      addName(stateVal, "state");

      // stack[stackIndexVal] = stateVal
      auto stackPtr = createAccessChain(spv::StorageClass::StorageClassOutput, stackVar(), { stackIdxVal });
      addName(stackPtr, "stackPtr");
      createStore(stateVal, stackPtr);

      // stackIndexVal += 1
      auto constPushCount = makeUintConstant(1);
      auto newStackIdxVal = createBinOp(spv::Op::OpIAdd, uintType(), stackIdxVal, constPushCount);
      this->addName(newStackIdxVal, "newStackIdx");

      // *stackIndexVar = stackIndexVal
      createStore(newStackIdxVal, stackIndexVar());
   }

   void popStack(int popCount)
   {
      // stateIdxVal = *stackIndexVar
      auto stateIdxVal = createLoad(stackIndexVar());
      addName(stateIdxVal, "stackIdx");

      if (popCount > 0) {
         // stateIdxVal -= {popCount}
         auto constPopCount = makeUintConstant(popCount);
         auto newStackIdxVal = createBinOp(spv::Op::OpISub, uintType(), stateIdxVal, constPopCount);
         addName(newStackIdxVal, "newStackIdx");

         // *stackIndexVar = stateIdxVal
         createStore(newStackIdxVal, stackIndexVar());
      }

      // newStateVal = stack[stackIndexVal]
      auto stackPtr = createAccessChain(spv::StorageClass::StorageClassInput, stackVar(), { stateIdxVal });
      addName(stackPtr, "stackPtr");
      auto newStateVal = createLoad(stackPtr);
      addName(newStateVal, "newState");

      // *stateVar = newStateVal
      createStore(newStateVal, stateVar());
   }

   spv::Block *startCfCondBlock(latte::SQ_CF_COND cond = latte::SQ_CF_COND::ACTIVE,
                                uint32_t condConst = 0)
   {
      // TODO: Support other cond types
      // Requires implementing the CF_CONST registers
      decaf_check(cond == latte::SQ_CF_COND::ACTIVE);

      auto insideBlock = &makeNewBlock();
      auto afterBlock = &makeNewBlock();

      auto state = createLoad(stateVar());
      auto pred = createBinOp(spv::OpIEqual, boolType(), state, stateActive());

      createSelectionMerge(afterBlock, spv::SelectionControlMaskNone);
      createConditionalBranch(pred, insideBlock, afterBlock);

      setBuildPoint(insideBlock);
      return afterBlock;
   }

   void endCfCondBlock(spv::Block *afterBlock, bool blockReturned = false)
   {
      if (!blockReturned) {
         createBranch(afterBlock);
      }
      setBuildPoint(afterBlock);
   }

   spv::Id readChanSel(spv::Id input, latte::SQ_CHAN chan)
   {
      decaf_check(chan != latte::SQ_CHAN::T);
      return createCompositeExtract(input, floatType(), { static_cast<unsigned int>(chan) });
   }

   spv::Id getGprRefGprIndex(const latte::GprRef &gpr)
   {
      spv::Id regIdx;

      // regIdx = {gprNumber}
      auto constGprNum = makeUintConstant(gpr.number);
      regIdx = constGprNum;

      switch (gpr.indexMode) {
      case latte::GprIndexMode::None:
      {
         // for R[{gprNumber}]
         break;
      }
      case latte::GprIndexMode::AR_X:
      {
         // for R[{gprNumber} + AR.x]

         // ARx = $AR.x
         auto ARxVal = getArId(SQ_CHAN::X);

         // regIdx = regIdx + ARx
         auto gprNumARx = createBinOp(spv::Op::OpIAdd, uintType(), regIdx, ARxVal);
         addName(gprNumARx, "gprNum");
         regIdx = gprNumARx;

         break;
      }
      case latte::GprIndexMode::AL:
      {
         // for R[{gprNumber} + AL]

         // AL = *ALVar
         auto ALVal = createLoad(ALVar());
         addName(ALVal, "AL");

         // regIdx = regIdx + AL
         regIdx = createBinOp(spv::Op::OpIAdd, uintType(), regIdx, ALVal);
         break;
      }
      default:
         decaf_abort("Unexpected GPR index mode");
      }

      return regIdx;
   }

   spv::Id getGprRef(const latte::GprRef &gpr)
   {
      auto regIdx = getGprRefGprIndex(gpr);

      // $ = &R[regIdx]
      return createAccessChain(spv::StorageClass::StorageClassPrivate, gprVar(), { regIdx });
   }

   spv::Id readGprChanRef(const latte::GprChanRef &ref)
   {
      auto regIdxVal = getGprRefGprIndex(ref.gpr);

      decaf_check(ref.chan != latte::SQ_CHAN::T);
      auto chanConst = makeUintConstant(static_cast<unsigned int>(ref.chan));

      // $ = R[regIdx].{refChan}
      auto gprChanPtr = createAccessChain(spv::StorageClass::StorageClassPrivate, gprVar(), { regIdxVal, chanConst });
      return createLoad(gprChanPtr);
   }

   spv::Id readGprMaskRef(const latte::GprMaskRef &ref)
   {
      // Read the source GPR
      auto srcGprPtr = getGprRef(ref.gpr);
      auto srcGprVal = createLoad(srcGprPtr);

      // Apply the source GPR swizzling
      decaf_check(isSwizzleFullyUnmasked(ref.mask));
      return applySelMask(spv::NoResult, srcGprVal, ref.mask);
   }

   spv::Id getCbufferRefIndex(const latte::CbufferRef &ref)
   {
      auto indexVal = makeIntConstant(ref.index);

      switch (ref.indexMode) {
      case latte::CbufferIndexMode::None:
         // This is the default mode...
         break;
      case latte::CbufferIndexMode::AL:
         indexVal = createBinOp(spv::OpIAdd, intType(), indexVal, createLoad(ALVar()));
         break;
      default:
         decaf_abort("Unexpected cfile index mode");
      }

      return indexVal;
   }

   spv::Id readCbufferChanRef(const latte::CbufferChanRef &ref)
   {
      auto thisCbufVar = cbufferVar(ref.cbuffer.bufferId);
      auto cbufIdxVal = getCbufferRefIndex(ref.cbuffer);

      decaf_check(ref.chan != latte::SQ_CHAN::T);
      auto chanConst = makeUintConstant(static_cast<unsigned int>(ref.chan));

      // $ = CBUFFER{bufferindex}[index].{refChan}
      auto zeroConst = makeUintConstant(0);
      auto cfileChanPtr = createAccessChain(spv::StorageClass::StorageClassUniform, thisCbufVar, { zeroConst, cbufIdxVal, chanConst });
      return createLoad(cfileChanPtr);
   }

   spv::Id getCfileRefIndex(const latte::CfileRef &ref)
   {
      auto indexVal = makeIntConstant(ref.index);

      switch (ref.indexMode) {
      case latte::CfileIndexMode::None:
         // This is the default mode...
         break;
      case latte::CfileIndexMode::AR_X:
         indexVal = createBinOp(spv::OpIAdd, intType(), indexVal, getArId(SQ_CHAN::X));
         break;
      case latte::CfileIndexMode::AR_Y:
         indexVal = createBinOp(spv::OpIAdd, intType(), indexVal, getArId(SQ_CHAN::Y));
         break;
      case latte::CfileIndexMode::AR_Z:
         indexVal = createBinOp(spv::OpIAdd, intType(), indexVal, getArId(SQ_CHAN::Z));
         break;
      case latte::CfileIndexMode::AR_W:
         indexVal = createBinOp(spv::OpIAdd, intType(), indexVal, getArId(SQ_CHAN::W));
         break;
      case latte::CfileIndexMode::AL:
         indexVal = createBinOp(spv::OpIAdd, intType(), indexVal, createLoad(ALVar()));
         break;
      default:
         decaf_abort("Unexpected cfile index mode");
      }

      return indexVal;
   }

   spv::Id getCfileRef(const latte::CfileRef &ref)
   {
      auto cfileIdxVal = getCfileRefIndex(ref);

      // $ = &CFILE[cfileIdx]
      auto zeroConst = makeUintConstant(0);
      return createAccessChain(spv::StorageClass::StorageClassUniform, cfileVar(), { zeroConst, cfileIdxVal });
   }

   spv::Id readCfileChanRef(const latte::CfileChanRef &ref)
   {
      auto cfileIdxVal = getCfileRefIndex(ref.cfile);

      decaf_check(ref.chan != latte::SQ_CHAN::T);
      auto chanConst = makeUintConstant(static_cast<unsigned int>(ref.chan));

      // $ = CFILE[index].{refChan}
      auto zeroConst = makeUintConstant(0);
      auto cfileChanPtr = createAccessChain(spv::StorageClass::StorageClassUniform, cfileVar(), { zeroConst, cfileIdxVal, chanConst });
      return createLoad(cfileChanPtr);
   }

   spv::Id readSrcVarRef(const SrcVarRef& source)
   {
      spv::Id srcId;

      if (source.type == latte::SrcVarRef::Type::GPR) {
         srcId = readGprChanRef(source.gprChan);
      } else if (source.type == latte::SrcVarRef::Type::CBUFFER) {
         srcId = readCbufferChanRef(source.cbufferChan);
      } else if (source.type == latte::SrcVarRef::Type::CFILE) {
         srcId = readCfileChanRef(source.cfileChan);
      } else if (source.type == latte::SrcVarRef::Type::PREVRES) {
         decaf_check(source.prevres.unit >= latte::SQ_CHAN::X);
         decaf_check(source.prevres.unit <= latte::SQ_CHAN::T);
         srcId = getPvId(source.prevres.unit);
      } else if (source.type == latte::SrcVarRef::Type::VALUE) {
         if (source.valueType == latte::VarRefType::FLOAT) {
            srcId = makeFloatConstant(source.value.floatValue);
         } else if (source.valueType == latte::VarRefType::INT) {
            srcId = makeIntConstant(source.value.intValue);
         } else if (source.valueType == latte::VarRefType::UINT) {
            srcId = makeUintConstant(source.value.uintValue);
         } else {
            decaf_abort("Unexpected source value type");
         }
      } else {
         decaf_abort("Unexpected source var type");
      }

      if (source.valueType == latte::VarRefType::FLOAT) {
         // We are naturally a float for SPIRV conversion
      } else if (source.valueType == latte::VarRefType::INT) {
         srcId = createUnaryOp(spv::Op::OpBitcast, intType(), srcId);
      } else if (source.valueType == latte::VarRefType::UINT) {
         srcId = createUnaryOp(spv::Op::OpBitcast, uintType(), srcId);
      } else {
         decaf_abort("Unexpected source value type");
      }

      if (source.isAbsolute) {
         if (getTypeId(srcId) == intType()) {
            srcId = createBuiltinCall(intType(), glslStd450(), GLSLstd450::GLSLstd450SAbs, { srcId });
         } else if (getTypeId(srcId) == floatType()) {
            srcId = createBuiltinCall(floatType(), glslStd450(), GLSLstd450::GLSLstd450FAbs, { srcId });
         } else {
            decaf_abort("unsupported source type for absolution");
         }
      }

      if (source.isNegated) {
         if (getTypeId(srcId) == intType()) {
            srcId = createUnaryOp(spv::Op::OpSNegate, intType(), srcId);
         } else if (getTypeId(srcId) == floatType()) {
            srcId = createUnaryOp(spv::Op::OpFNegate, floatType(), srcId);
         } else {
            decaf_abort("unsupported source type for negation");
         }
      }

      return srcId;
   }

   spv::Id readAluInstSrc(const latte::ControlFlowInst &cf, const latte::AluInstructionGroup &group, const latte::AluInst &inst,
                      uint32_t srcIndex, latte::VarRefType valueType = latte::VarRefType::FLOAT)
   {
      auto source = makeSrcVar(cf, group, inst, srcIndex, valueType);
      return readSrcVarRef(source);
   }

   spv::Id readAluReducSrc(const latte::ControlFlowInst &cf, const latte::AluInstructionGroup &group,
                           uint32_t srcIndex, latte::VarRefType valueType = latte::VarRefType::FLOAT)
   {
      spv::Id resultType;
      if (valueType == VarRefType::FLOAT) {
         resultType = float4Type();
      } else if (valueType == VarRefType::UINT) {
         resultType = uint4Type();
      } else if (valueType == VarRefType::INT) {
         resultType = int4Type();
      } else {
         decaf_abort("Unexpected value type");
      }

      auto srcX = makeSrcVar(cf, group, *group.units[SQ_CHAN::X], srcIndex, valueType);
      auto srcY = makeSrcVar(cf, group, *group.units[SQ_CHAN::Y], srcIndex, valueType);
      auto srcZ = makeSrcVar(cf, group, *group.units[SQ_CHAN::Z], srcIndex, valueType);
      auto srcW = makeSrcVar(cf, group, *group.units[SQ_CHAN::W], srcIndex, valueType);

      // In order to improve our shader debugging experience, if we are simply
      // fetching the entirety of a register, lets fetch it all at once, instead
      // of doing it component by component...  There is actually an opportunity
      // to allow swizzling here too.  Thats probably unneccessary though.
      if (srcX.type == SrcVarRef::Type::GPR &&
          srcY.type == SrcVarRef::Type::GPR &&
          srcZ.type == SrcVarRef::Type::GPR &&
          srcW.type == SrcVarRef::Type::GPR &&
          srcX.gprChan.chan == latte::SQ_CHAN::X &&
          srcY.gprChan.chan == latte::SQ_CHAN::Y &&
          srcZ.gprChan.chan == latte::SQ_CHAN::Z &&
          srcW.gprChan.chan == latte::SQ_CHAN::W &&
          (srcX.gprChan.gpr.number == srcY.gprChan.gpr.number) &&
          (srcY.gprChan.gpr.number == srcZ.gprChan.gpr.number) &&
          (srcZ.gprChan.gpr.number == srcW.gprChan.gpr.number) &&
          (srcX.gprChan.gpr.indexMode == srcY.gprChan.gpr.indexMode) &&
          (srcY.gprChan.gpr.indexMode == srcZ.gprChan.gpr.indexMode) &&
          (srcZ.gprChan.gpr.indexMode == srcW.gprChan.gpr.indexMode))
      {
         auto srcRegPtr = getGprRef(srcX.gprChan.gpr);
         return createLoad(srcRegPtr);
      }

      if (srcX.type == SrcVarRef::Type::CFILE &&
          srcY.type == SrcVarRef::Type::CFILE &&
          srcZ.type == SrcVarRef::Type::CFILE &&
          srcW.type == SrcVarRef::Type::CFILE &&
          srcX.cfileChan.chan == latte::SQ_CHAN::X &&
          srcY.cfileChan.chan == latte::SQ_CHAN::Y &&
          srcZ.cfileChan.chan == latte::SQ_CHAN::Z &&
          srcW.cfileChan.chan == latte::SQ_CHAN::W &&
          srcX.cfileChan.cfile.index == srcY.cfileChan.cfile.index &&
          srcY.cfileChan.cfile.index == srcZ.cfileChan.cfile.index &&
          srcZ.cfileChan.cfile.index == srcW.cfileChan.cfile.index &&
          srcX.cfileChan.cfile.indexMode == srcY.cfileChan.cfile.indexMode &&
          srcY.cfileChan.cfile.indexMode == srcZ.cfileChan.cfile.indexMode &&
          srcZ.cfileChan.cfile.indexMode == srcW.cfileChan.cfile.indexMode)
      {
         auto srcCfilePtr = getCfileRef(srcX.cfileChan.cfile);
         return createLoad(srcCfilePtr);
      }

      return createCompositeConstruct(resultType, {
            readSrcVarRef(srcX),
            readSrcVarRef(srcY),
            readSrcVarRef(srcZ),
            readSrcVarRef(srcW) });
   }

   void writeGprChanRef(const GprChanRef& ref, spv::Id srcId)
   {
      auto regIdx = getGprRefGprIndex(ref.gpr);

      decaf_check(ref.chan != latte::SQ_CHAN::T);
      auto chanConst = makeUintConstant(static_cast<unsigned int>(ref.chan));

      // $ = R[regIdx].{refChan}
      auto gprChanPtr = createAccessChain(spv::StorageClass::StorageClassPrivate, gprVar(), { regIdx, chanConst });
      createStore(srcId, gprChanPtr);
   }

   void writeGprMaskRef(const latte::GprMaskRef &ref, spv::Id srcId)
   {
      auto gprRef = getGprRef(ref.gpr);

      // We must put the srcId here, since some swizzles will just be rearranging
      auto writeVal = srcId;

      // If the swizzle masks anything, we need to load the original value
      // of the export so that we can preserve that data not being written.
      if (!isSwizzleFullyUnmasked(ref.mask)) {
         writeVal = createLoad(gprRef);
      }

      writeVal = applySelMask(writeVal, srcId, ref.mask);

      createStore(writeVal, gprRef);
   }

   spv::Id applySelMask(spv::Id dest, spv::Id source, std::array<SQ_SEL, 4> mask)
   {
      // If the swizzle is just XYZW, we don't actually need to do anything...
      bool isNoop = true;
      isNoop &= (mask[0] == latte::SQ_SEL::SEL_X);
      isNoop &= (mask[1] == latte::SQ_SEL::SEL_Y);
      isNoop &= (mask[2] == latte::SQ_SEL::SEL_Z);
      isNoop &= (mask[3] == latte::SQ_SEL::SEL_W);
      if (isNoop) {
         return source;
      }

      // If the swizzle is ____, we should just skip processing entirely
      bool isMasked = true;
      isMasked &= (mask[0] == latte::SQ_SEL::SEL_MASK);
      isMasked &= (mask[1] == latte::SQ_SEL::SEL_MASK);
      isMasked &= (mask[2] == latte::SQ_SEL::SEL_MASK);
      isMasked &= (mask[3] == latte::SQ_SEL::SEL_MASK);
      if (isMasked) {
         return dest;
      }

      // Lets see if this swizzle is using constants at all, since we can optimize
      // away the need for an intermediary vector
      bool usesConstants = false;
      for (auto selIdx = 0; selIdx < 4; ++selIdx) {
         switch (mask[selIdx]) {
         case latte::SQ_SEL::SEL_X:
            break;
         case latte::SQ_SEL::SEL_Y:
            break;
         case latte::SQ_SEL::SEL_Z:
            break;
         case latte::SQ_SEL::SEL_W:
            break;
         case latte::SQ_SEL::SEL_0:
            usesConstants = true;
            break;
         case latte::SQ_SEL::SEL_1:
            usesConstants = true;
            break;
         case latte::SQ_SEL::SEL_MASK:
            break;
         default:
            decaf_abort("Unexpected selector during masking operation.");
         }
      }

      if (!usesConstants) {
         // If the swizzle isn't using constants, we can just directly shuffle
         // between the two input vectors in a single go.

         std::array<unsigned int, 4> shuffleIdx = { 0, 1, 2, 3 };
         for (auto selIdx = 0; selIdx < 4; ++selIdx) {
            switch (mask[selIdx]) {
            case latte::SQ_SEL::SEL_X:
               shuffleIdx[selIdx] = 4;
               break;
            case latte::SQ_SEL::SEL_Y:
               shuffleIdx[selIdx] = 5;
               break;
            case latte::SQ_SEL::SEL_Z:
               shuffleIdx[selIdx] = 6;
               break;
            case latte::SQ_SEL::SEL_W:
               shuffleIdx[selIdx] = 7;
               break;
            case latte::SQ_SEL::SEL_0:
               usesConstants = true;
               break;
            case latte::SQ_SEL::SEL_1:
               usesConstants = true;
               break;
            case latte::SQ_SEL::SEL_MASK:
               break;
            default:
               decaf_abort("Unexpected selector during masking operation.");
            }
         }

         return createOp(spv::Op::OpVectorShuffle, float4Type(), { dest, source, shuffleIdx[0], shuffleIdx[1], shuffleIdx[2], shuffleIdx[3] });
      }

      // If the swizzle is using constants, we need to pull out the individual pieces
      // and then build a new vector with all the values

      std::array<spv::Id, 4> resultElems = { spv::NoResult };

      // Because its possible to swizzle the same source channel into multiple places
      // of the destination, we keep a cache of the ones we've already extracted.
      std::array<spv::Id, 4> sourceElems = { spv::NoResult };
      auto fetchSrcElem = [&](unsigned int elemIdx)
      {
         auto& elem = sourceElems[elemIdx];
         if (!elem) {
            elem = createOp(spv::OpCompositeExtract, floatType(), { source, elemIdx });
         }
         return elem;
      };

      for (auto selIdx = 0u; selIdx < 4u; ++selIdx) {
         switch (mask[selIdx]) {
         case latte::SQ_SEL::SEL_X:
            resultElems[selIdx] = fetchSrcElem(0);
            break;
         case latte::SQ_SEL::SEL_Y:
            resultElems[selIdx] = fetchSrcElem(1);
            break;
         case latte::SQ_SEL::SEL_Z:
            resultElems[selIdx] = fetchSrcElem(2);
            break;
         case latte::SQ_SEL::SEL_W:
            resultElems[selIdx] = fetchSrcElem(3);
            break;
         case latte::SQ_SEL::SEL_0:
            resultElems[selIdx] = makeFloatConstant(0.0f);
            break;
         case latte::SQ_SEL::SEL_1:
            resultElems[selIdx] = makeFloatConstant(1.0f);
            break;
         case latte::SQ_SEL::SEL_MASK:
            resultElems[selIdx] = createOp(spv::OpCompositeExtract, floatType(), { dest, selIdx });
            break;
         default:
            decaf_abort("Unexpected selector during masking operation.");
         }
      }

      return createOp(spv::OpCompositeConstruct, float4Type(),
                      { resultElems[0], resultElems[1], resultElems[2], resultElems[3] });
   }

   void writeAluOpDest(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst, spv::Id srcId, bool forAr = false)
   {
      if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
         switch (inst.op2.OMOD()) {
         case SQ_ALU_OMOD::D2:
            decaf_check(getTypeId(srcId) == floatType());
            srcId = createBinOp(spv::Op::OpFDiv, floatType(), srcId, makeFloatConstant(2));
            break;
         case SQ_ALU_OMOD::M2:
            decaf_check(getTypeId(srcId) == floatType());
            srcId = createBinOp(spv::Op::OpFMul, floatType(), srcId, makeFloatConstant(2));
            break;
         case SQ_ALU_OMOD::M4:
            decaf_check(getTypeId(srcId) == floatType());
            srcId = createBinOp(spv::Op::OpFMul, floatType(), srcId, makeFloatConstant(4));
            break;
         case SQ_ALU_OMOD::OFF:
            // Nothing to do
            break;
         default:
            decaf_abort("Unexpected dest var OMOD");
         }
      }

      if (inst.word1.CLAMP()) {
         decaf_check(getTypeId(srcId) == floatType());
         srcId = createBuiltinCall(floatType(), glslStd450(), GLSLstd450::GLSLstd450FClamp, { srcId, makeFloatConstant(0), makeFloatConstant(1) });
      }

      if (forAr) {
         // Instruction is responsible for dispatching the result as a
         //  UINT automatically, so we can safely directly store it
         //  as AR is typed as UINT in the shader.
      } else {
         // Instruction returns the value in whatever type is intended
         //  by the instruction.  We use meta-data to translate that
         //  type to the float type used for storage in shader.
         auto flags = getInstructionFlags(inst);
         if (flags & SQ_ALU_FLAG_INT_OUT) {
            decaf_check(getTypeId(srcId) == intType());
         } else if (flags & SQ_ALU_FLAG_UINT_OUT) {
            decaf_check(getTypeId(srcId) == uintType());
         }

         auto srcTypeId = getTypeId(srcId);
         if (srcTypeId == floatType()) {
            // Nothing to do, we are already a float!
         } else if (srcTypeId == intType()) {
            srcId = createUnaryOp(spv::OpBitcast, floatType(), srcId);
         } else if (srcTypeId == uintType()) {
            srcId = createUnaryOp(spv::OpBitcast, floatType(), srcId);
         } else {
            decaf_abort("Unexpected type at ALU instruction write");
         }
      }

      // According to the documentation, AR/PV writes are only valid for their
      // respective instructions, and other accesses are illegal...
      if (forAr) {
         decaf_check(unit != latte::SQ_CHAN::T);
      }
      mNextPrevResId[unit] = srcId;

      if (inst.word1.ENCODING() != SQ_ALU_ENCODING::OP2 || inst.op2.WRITE_MASK()) {
         // According to the docs, GPR writes are undefined for AR instructions
         // though its worth noting that it does not actually say its illegal.
         decaf_check(!forAr);

         GprChanRef destGpr;
         destGpr.gpr = makeGprRef(inst.word1.DST_GPR(), inst.word1.DST_REL(), inst.word0.INDEX_MODE());
         destGpr.chan = inst.word1.DST_CHAN();
         writeGprChanRef(destGpr, srcId);
      }
   }

   void writeAluReducDest(const ControlFlowInst &cf, const AluInstructionGroup &group, spv::Id srcId, bool forAr = false)
   {
      // Reduction instructions occupy XYZW, but T is free for other operations.
      // Note that we intentionally select a default output unit of X here, this
      // ensures that if there is no specifically chosen unit, that this will mean
      // that X implicitly had its write-mask disabled, and the writeAluOpDest
      // function will elide the register write.  Note that no matter which unit
      // is writing, the result always goes to PV.x
      auto outputUnit = SQ_CHAN::X;
      for (auto i = 0u; i < 4u; ++i) {
         if (group.units[i]->op2.WRITE_MASK()) {
            outputUnit = static_cast<SQ_CHAN>(i);
            break;
         }
      }

      writeAluOpDest(cf, group, SQ_CHAN::X, *group.units[outputUnit], srcId, forAr);
   }

   void updatePredicateAndExecuteMask(const ControlFlowInst &cf, const AluInst &inst, spv::Id pred)
   {
      if (inst.op2.UPDATE_PRED()) {
         createStore(pred, predicateVar());
      }

      if (inst.op2.UPDATE_EXECUTE_MASK()) {
         createStore(createTriOp(spv::Op::OpSelect, intType(), pred,
                                 stateActive(), stateInactive()),
                     stateVar());
      }
   }

   spv::Id getExportRefVar(const ExportRef& ref)
   {
      if (ref.type == ExportRef::Type::Position) {
         return posExportVar(ref.index);
      } else if (ref.type == ExportRef::Type::Param) {
         return paramExportVar(ref.index);
      } else if (ref.type == ExportRef::Type::Pixel) {
         return pixelExportVar(ref.index);
      } else {
         decaf_abort("Encountered unexpected export type");
      }
   }

   spv::Id readExportRef(const ExportRef& ref)
   {
      return createLoad(getExportRefVar(ref));
   }

   void writeExportRef(const ExportRef& ref, std::array<SQ_SEL, 4> mask, spv::Id srcId)
   {
      auto exportRef = getExportRefVar(ref);

      // We must put the srcId here, since some swizzles will just be rearranging
      auto exportVal = srcId;

      // If the swizzle masks anything, we need to load the original value
      // of the export so that we can preserve that data not being written.
      if (!isSwizzleFullyUnmasked(mask)) {
         exportVal = createLoad(exportRef);
      }

      exportVal = applySelMask(exportVal, srcId, mask);

      if (ref.type == ExportRef::Type::Position) {
         // Need to reposition the depth values from (-1.0 to 1.0) to (0.0 to 1.0)

         auto zeroConst = makeUintConstant(0);
         auto oneConst = makeUintConstant(1);
         auto oneFConst = makeFloatConstant(1.0f);

         auto posMulPtr = createAccessChain(spv::StorageClass::StorageClassPushConstant, vsPushConstVar(), { zeroConst });
         auto posMulVal = createLoad(posMulPtr);

         auto posAddPtr = createAccessChain(spv::StorageClass::StorageClassPushConstant, vsPushConstVar(), { oneConst });
         auto posAddVal = createLoad(posAddPtr);

         auto exportW = createOp(spv::OpCompositeExtract, floatType(), { exportVal, 3 });
         auto finalNorm = createOp(spv::OpCompositeConstruct, float4Type(), { exportW, exportW, exportW, oneFConst });

         exportVal = createBinOp(spv::OpFMul, float4Type(), exportVal, posMulVal);
         exportVal = createBinOp(spv::OpFAdd, float4Type(), exportVal, posAddVal);
         exportVal = createBinOp(spv::OpFMul, float4Type(), exportVal, finalNorm);
      }

      if (ref.type == ExportRef::Type::Pixel) {
         // We use the first exported pixel to perform alpha reference testing.  This
         // may not actually be the correct behaviour.
         // TODO: Check which exported pixel does alpha testing.
         if (ref.index == 0) {
            auto exportAlpha = createOp(spv::OpCompositeExtract, floatType(), { exportVal, 3 });

            auto zeroConst = makeUintConstant(0);
            auto oneConst = makeUintConstant(1);

            auto alphaFuncPtr = createAccessChain(spv::StorageClassPushConstant, psPushConstVar(), { zeroConst });
            auto alphaFuncVal = createLoad(alphaFuncPtr);

            auto alphaRefPtr = createAccessChain(spv::StorageClassPushConstant, psPushConstVar(), { oneConst });
            auto alphaRefVal = createLoad(alphaRefPtr);

            auto makeCompareBlock = [&](spv::Op op)
            {
               auto pred = createBinOp(op, boolType(), exportAlpha, alphaRefVal);
               auto notPred = createUnaryOp(spv::Op::OpLogicalNot, boolType(), pred);
               auto block = spv::Builder::If { notPred, spv::SelectionControlMaskNone, *this };
               makeDiscard();
               block.makeEndIf();
            };

            auto switchSegments = std::vector<spv::Block *> { };
            makeSwitch(alphaFuncVal, spv::SelectionControlMaskNone, 7,
                       {
                           latte::REF_FUNC::NEVER,
                           latte::REF_FUNC::LESS,
                           latte::REF_FUNC::EQUAL,
                           latte::REF_FUNC::LESS_EQUAL,
                           latte::REF_FUNC::GREATER,
                           latte::REF_FUNC::NOT_EQUAL,
                           latte::REF_FUNC::GREATER_EQUAL,
                       },
                       { 0, 1, 2, 3, 4, 5, 6 }, -1, switchSegments);

            nextSwitchSegment(switchSegments, latte::REF_FUNC::NEVER);
            makeDiscard();
            addSwitchBreak();

            nextSwitchSegment(switchSegments, latte::REF_FUNC::LESS);
            makeCompareBlock(spv::OpFOrdLessThan);
            addSwitchBreak();

            nextSwitchSegment(switchSegments, latte::REF_FUNC::EQUAL);
            makeCompareBlock(spv::OpFOrdEqual);
            addSwitchBreak();

            nextSwitchSegment(switchSegments, latte::REF_FUNC::LESS_EQUAL);
            makeCompareBlock(spv::OpFOrdLessThanEqual);
            addSwitchBreak();

            nextSwitchSegment(switchSegments, latte::REF_FUNC::GREATER);
            makeCompareBlock(spv::OpFOrdGreaterThan);
            addSwitchBreak();

            nextSwitchSegment(switchSegments, latte::REF_FUNC::NOT_EQUAL);
            makeCompareBlock(spv::OpFOrdNotEqual);
            addSwitchBreak();

            nextSwitchSegment(switchSegments, latte::REF_FUNC::GREATER_EQUAL);
            makeCompareBlock(spv::OpFOrdGreaterThanEqual);
            addSwitchBreak();

            endSwitch(switchSegments);
         }
      }

      createStore(exportVal, exportRef);
   }

   spv::Id vertexIdVar()
   {
      if (!mVertexId) {
         mVertexId = createVariable(spv::StorageClassInput, intType(), "VertexID");
         addDecoration(mVertexId, spv::DecorationBuiltIn, spv::BuiltInVertexId);
         mEntryPoint->addIdOperand(mVertexId);
      }
      return mVertexId;
   }

   spv::Id instanceIdVar()
   {
      if (!mInstanceId) {
         mInstanceId = createVariable(spv::StorageClassInput, intType(), "InstanceID");
         addDecoration(mInstanceId, spv::DecorationBuiltIn, spv::BuiltInInstanceId);
         mEntryPoint->addIdOperand(mInstanceId);
      }
      return mInstanceId;
   }

   spv::Id inputAttribVar(int semLocation, spv::Id attribType)
   {
      auto attribIdx = mAttribInputs.size();

      auto attribVar = createVariable(spv::StorageClassInput, attribType,
                                      fmt::format("INPUT_{}", attribIdx).c_str());
      addDecoration(attribVar, spv::DecorationLocation, static_cast<int>(semLocation));

      mEntryPoint->addIdOperand(attribVar);
      mAttribInputs.push_back(attribVar);

      return attribVar;
   }

   spv::Id inputParamVar(int semLocation)
   {
      auto paramIdx = mParamInputs.size();

      auto inputVar = createVariable(spv::StorageClassInput, float4Type(),
                                     fmt::format("PARAM_{}", paramIdx).c_str());
      addDecoration(inputVar, spv::DecorationLocation, static_cast<int>(semLocation));

      mEntryPoint->addIdOperand(inputVar);
      mParamInputs.push_back(inputVar);

      return inputVar;
   }

   spv::Id vsPushConstVar()
   {
      if (!mVsPushConsts) {
         auto vsPushStruct = makeStructType({ float4Type(), float4Type() }, "VS_PUSH_CONSTANTS");
         addMemberDecoration(vsPushStruct, 0, spv::DecorationOffset, 0);
         addMemberName(vsPushStruct, 0, "posMul");
         addMemberDecoration(vsPushStruct, 1, spv::DecorationOffset, 16);
         addMemberName(vsPushStruct, 1, "posAdd");
         addDecoration(vsPushStruct, spv::DecorationBlock);
         mVsPushConsts = createVariable(spv::StorageClassPushConstant, vsPushStruct, "VS_PUSH");
      }
      return mVsPushConsts;
   }

   spv::Id gsPushConstVar()
   {
      decaf_abort("There are not geometry shader push constants");
   }

   spv::Id psPushConstVar()
   {
      if (!mPsPushConsts) {
         auto psPushStruct = makeStructType({ uintType(), floatType() }, "PS_PUSH_DATA");
         addMemberDecoration(psPushStruct, 0, spv::DecorationOffset, 32 + 0);
         addMemberName(psPushStruct, 0, "alphaFunc");
         addMemberDecoration(psPushStruct, 1, spv::DecorationOffset, 32 + 4);
         addMemberName(psPushStruct, 1, "alphaRef");
         addDecoration(psPushStruct, spv::DecorationBlock);
         mPsPushConsts = createVariable(spv::StorageClassPushConstant, psPushStruct, "PS_PUSH");
      }
      return mPsPushConsts;
   }

   spv::Id cfileVar()
   {
      if (!mRegistersBuffer) {
         auto regsType = arrayType(float4Type(), 256);
         addDecoration(regsType, spv::DecorationArrayStride, 16);
         auto structType = this->makeStructType({ regsType }, "CFILE_DATA");
         addMemberDecoration(structType, 0, spv::DecorationOffset, 0);
         addDecoration(structType, spv::DecorationBlock);
         addMemberName(structType, 0, "values");

         mRegistersBuffer = createVariable(spv::StorageClassUniform, structType, "CFILE");
         addDecoration(mRegistersBuffer, spv::DecorationDescriptorSet, mDescriptorSetIndex);
         addDecoration(mRegistersBuffer, spv::DecorationBinding, latte::MaxSamplers + latte::MaxTextures + 0);
      }

      return mRegistersBuffer;
   }

   spv::Id cbufferVar(uint32_t cbufferIdx)
   {
      decaf_check(cbufferIdx < latte::MaxUniformBlocks);

      auto cbuffer = mUniformBuffers[cbufferIdx];
      if (!cbuffer) {
         auto bufferType = arrayType(float4Type(), 0);
         cbuffer = createVariable(spv::StorageClassUniform, bufferType, fmt::format("CBUFFER_{}", cbufferIdx).c_str());
         addDecoration(cbuffer, spv::DecorationDescriptorSet, mDescriptorSetIndex);
         addDecoration(cbuffer, spv::DecorationBinding, latte::MaxSamplers + latte::MaxTextures + cbufferIdx);

         mUniformBuffers[cbufferIdx] = cbuffer;
      }

      return cbuffer;
   }

   spv::Id samplerVar(uint32_t samplerIdx)
   {
      decaf_check(samplerIdx < latte::MaxSamplers);

      auto samplerId = mSamplers[samplerIdx];
      if (!samplerId) {
         samplerId = createVariable(spv::StorageClassUniformConstant, samplerType());
         addName(samplerId, fmt::format("SAMPLER_{}", samplerIdx).c_str());
         addDecoration(samplerId, spv::DecorationDescriptorSet, mDescriptorSetIndex);
         addDecoration(samplerId, spv::DecorationBinding, samplerIdx);

         mSamplers[samplerIdx] = samplerId;
      }

      return samplerId;
   }

   spv::Id textureVarType(uint32_t textureIdx)
   {
      decaf_check(textureIdx < latte::MaxTextures);

      auto textureType = mTextureTypes[textureIdx];
      if (!textureType) {
         textureType = makeImageType(floatType(), spv::Dim2D, false, false, false, 1, spv::ImageFormatUnknown);

         mTextureTypes[textureIdx] = textureType;
      }

      return textureType;
   }

   spv::Id textureVar(uint32_t textureIdx)
   {
      decaf_check(textureIdx < latte::MaxTextures);

      auto textureId = mTextures[textureIdx];
      if (!textureId) {
         textureId = createVariable(spv::StorageClassUniformConstant, textureVarType(textureIdx));
         addName(textureId, fmt::format("TEXTURE_{}", textureIdx).c_str());
         addDecoration(textureId, spv::DecorationDescriptorSet, mDescriptorSetIndex);
         addDecoration(textureId, spv::DecorationBinding, latte::MaxSamplers + textureIdx);

         mTextures[textureIdx] = textureId;
      }

      return textureId;
   }

   spv::Id pixelExportVar(uint32_t pixelIdx)
   {
      decaf_check(pixelIdx < latte::MaxRenderTargets);

      while (mPixelExports.size() <= pixelIdx) {
         mPixelExports.push_back(spv::NoResult);
      }

      auto exportId = mPixelExports[pixelIdx];
      if (!exportId) {
         exportId = createVariable(spv::StorageClass::StorageClassOutput, float4Type(), fmt::format("PIXEL_{}", pixelIdx).c_str());
         addDecoration(exportId, spv::Decoration::DecorationLocation, pixelIdx);
         mPixelExports[pixelIdx] = exportId;

         mEntryPoint->addIdOperand(exportId);
      }

      return exportId;
   }

   spv::Id posExportVar(uint32_t posIdx)
   {
      decaf_check(posIdx < 4);

      while (mPosExports.size() <= posIdx) {
         mPosExports.push_back(spv::NoResult);
      }

      auto exportId = mPosExports[posIdx];
      if (!exportId) {
         exportId = createVariable(spv::StorageClass::StorageClassOutput, float4Type(), fmt::format("POS_{}", posIdx).c_str());

         if (posIdx == 0) {
            // Mark this as being the position output
            addDecoration(exportId, spv::DecorationBuiltIn, spv::BuiltInPosition);
         } else {
            // Mark this to a location, not sure if this is really needed
            addDecoration(exportId, spv::Decoration::DecorationLocation, 60 + posIdx);
         }

         mPosExports[posIdx] = exportId;

         mEntryPoint->addIdOperand(exportId);
      }

      return exportId;
   }

   spv::Id paramExportVar(uint32_t paramIdx)
   {
      decaf_check(paramIdx < 32);

      while (mParamExports.size() <= paramIdx) {
         mParamExports.push_back(spv::NoResult);
      }

      auto exportId = mParamExports[paramIdx];
      if (!exportId) {
         exportId = createVariable(spv::StorageClass::StorageClassOutput, float4Type(), fmt::format("PARAM_{}", paramIdx).c_str());
         addDecoration(exportId, spv::Decoration::DecorationLocation, paramIdx);
         mParamExports[paramIdx] = exportId;

         mEntryPoint->addIdOperand(exportId);
      }

      return exportId;
   }

   spv::Id stateVar()
   {
      if (!mState) {
         mState = createVariable(spv::StorageClass::StorageClassPrivate, intType(), "stateVar");
      }
      return mState;
   }

   spv::Id stateActive()
   {
      return makeIntConstant(0);
   }

   spv::Id stateInactive()
   {
      return makeIntConstant(1);
   }

   spv::Id stateInactiveBreak()
   {
      return makeIntConstant(2);
   }

   spv::Id stateInactiveContinue()
   {
      return makeIntConstant(3);
   }

   spv::Id predicateVar()
   {
      if (!mPredicate) {
         mPredicate = createVariable(spv::StorageClass::StorageClassPrivate, boolType(), "predVar");
      }
      return mPredicate;
   }

   spv::Id stackVar()
   {
      if (!mStack) {
         auto stackType = makeArrayType(intType(), makeUintConstant(16), 0);
         mStack = createVariable(spv::StorageClass::StorageClassPrivate, stackType, "stackVar");
      }
      return mStack;
   }

   spv::Id stackIndexVar()
   {
      if (!mStackIndex) {
         mStackIndex = createVariable(spv::StorageClass::StorageClassPrivate, intType(), "stackIdxVar");
      }
      return mStackIndex;
   }

   spv::Id gprVar()
   {
      if (!mGpr) {
         auto gprType = makeArrayType(float4Type(), makeUintConstant(128), 0);
         mGpr = createVariable(spv::StorageClass::StorageClassPrivate, gprType, "RVar");
      }
      return mGpr;
   }

   spv::Id ALVar()
   {
      if (!mAL) {
         mAL = createVariable(spv::StorageClass::StorageClassPrivate, intType(), "ALVar");
      }
      return mAL;
   }

   void swapPrevRes()
   {
      // Make the next prevRes active
      mPrevResId = mNextPrevResId;

      // Clear the next prevRes's
      for (auto i = 0; i < 5; ++i) {
         mNextPrevResId[i] = spv::NoResult;
      }
   }

   spv::Id getPvId(latte::SQ_CHAN unit)
   {
      auto pvVal = mPrevResId[unit];
      decaf_check(getTypeId(pvVal) == floatType());
      return pvVal;
   }

   spv::Id getArId(latte::SQ_CHAN unit)
   {
      decaf_check(unit != latte::SQ_CHAN::T);

      auto arVal = mPrevResId[unit];
      decaf_check(getTypeId(arVal) == intType());
      return arVal;
   }

   bool hasFunction(const std::string& name) const
   {
      return mFunctions.find(name) != mFunctions.end();
   }

   spv::Function * getFunction(const std::string& name)
   {
      auto& func = mFunctions[name];
      if (!func) {
         auto savePos = getBuildPoint();
         func = makeEntryPoint(name.c_str());
         setBuildPoint(savePos);
      }
      return func;
   }

   bool isSamplerUsed(uint32_t samplerIdx) const
   {
      decaf_check(samplerIdx <= latte::MaxSamplers);
      return mSamplers[samplerIdx] != spv::NoResult;
   }

   bool isTextureUsed(uint32_t textureIdx) const
   {
      decaf_check(textureIdx <= latte::MaxTextures);
      return mTextures[textureIdx] != spv::NoResult;
   }

   bool isConstantFileUsed() const
   {
      return mRegistersBuffer != spv::NoResult;
   }

   bool isUniformBufferUsed(uint32_t bufferIdx) const
   {
      decaf_check(bufferIdx <= latte::MaxUniformBlocks);
      return mUniformBuffers[bufferIdx] != spv::NoResult;
   }

   int getNumPosExports()
   {
      return (int)mPosExports.size();
   }

   int getNumParamExports()
   {
      return (int)mParamExports.size();
   }

   int getNumPixelExports()
   {
      return (int)mPixelExports.size();
   }

protected:
   int mDescriptorSetIndex = 0;
   spv::Instruction *mEntryPoint = nullptr;
   std::unordered_map<std::string, spv::Function*> mFunctions;

   spv::Id mVertexId = spv::NoResult;
   spv::Id mInstanceId = spv::NoResult;

   spv::Id mRegistersBuffer = spv::NoResult;
   std::array<spv::Id, latte::MaxUniformBlocks> mUniformBuffers = { spv::NoResult };

   spv::Id mVsPushConsts = spv::NoResult;
   spv::Id mGsPushConsts = spv::NoResult;
   spv::Id mPsPushConsts = spv::NoResult;

   std::vector<spv::Id> mAttribInputs;
   std::vector<spv::Id> mParamInputs;

   std::array<spv::Id, latte::MaxSamplers> mSamplers = { spv::NoResult };
   std::array<spv::Id, latte::MaxTextures> mTextureTypes = { spv::NoResult };
   std::array<spv::Id, latte::MaxTextures> mTextures = { spv::NoResult };

   std::vector<spv::Id> mPixelExports;
   std::vector<spv::Id> mPosExports;
   std::vector<spv::Id> mParamExports;

   spv::Id mState = spv::NoResult;
   spv::Id mPredicate = spv::NoResult;
   spv::Id mStackIndex = spv::NoResult;
   spv::Id mStack = spv::NoResult;
   spv::Id mGpr = spv::NoResult;
   spv::Id mAL = spv::NoResult;

   std::array<spv::Id, 5> mPrevResId = { spv::NoResult };
   std::array<spv::Id, 5> mNextPrevResId = { spv::NoResult };

};

} //namespace spirv

#endif // ifdef DECAF_VULKAN
