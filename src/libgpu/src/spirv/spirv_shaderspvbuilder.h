#pragma once
#ifdef DECAF_VULKAN
#include "latte/latte_shaderparser.h"
#include "spirv_spvbuilder.h"
#include "gpu_config.h"

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

   void setBindingBase(int bindingBase)
   {
      mBindingBase = bindingBase;
   }

   void elseStack()
   {
      // stackIndexVal = *stackIndexVar
      auto stackIdxVal = createLoad(stackIndexVar());
      addName(stackIdxVal, "stackIdx");

      // prevStackIdxVal = stackIndexVal - 1
      auto oneConst = makeIntConstant(1);
      auto prevStackIdxVal = createBinOp(spv::OpISub, intType(), stackIdxVal, oneConst);

      // prevStateVal = stack[stackIndexVal]
      auto prevStackPtr = createAccessChain(spv::StorageClass::StorageClassPrivate, stackVar(), { prevStackIdxVal });
      addName(prevStackPtr, "prevStackPtr");
      auto prevStateVal = createLoad(prevStackPtr);
      addName(prevStateVal, "prevStackVal");

      // if (prevStackVal == Active) {
      auto isActive = createBinOp(spv::OpIEqual, boolType(),
                                  prevStateVal,
                                  stateActive());
      auto ifBuilder = spv::Builder::If { isActive, spv::SelectionControlMaskNone, *this };

      // state = *stateVar
      auto stateVal = createLoad(stateVar());
      addName(stateVal, "state");

      // newState = (state == Active) ? InactiveBreak : Active
      auto pred = createBinOp(spv::OpIEqual, boolType(), stateVal, stateActive());
      auto newState = createTriOp(spv::OpSelect, intType(), pred, stateInactive(), stateActive());

      // *stateVar = newState
      createStore(newState, stateVar());

      // }
      ifBuilder.makeEndIf();
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
      auto stackPtr = createAccessChain(spv::StorageClass::StorageClassPrivate, stackVar(), { stackIdxVal });
      addName(stackPtr, "stackPtr");
      createStore(stateVal, stackPtr);

      // stackIndexVal += 1
      auto constPushCount = makeIntConstant(1);
      auto newStackIdxVal = createBinOp(spv::Op::OpIAdd, intType(), stackIdxVal, constPushCount);
      this->addName(newStackIdxVal, "newStackIdx");

      // *stackIndexVar = stackIndexVal
      createStore(newStackIdxVal, stackIndexVar());
   }

   void popStack(int popCount)
   {
      // stateIdxVal = *stackIndexVar
      auto stackIdxVal = createLoad(stackIndexVar());
      addName(stackIdxVal, "stackIdx");

      if (popCount > 0) {
         // stateIdxVal -= {popCount}
         auto constPopCount = makeIntConstant(popCount);
         auto newStackIdxVal = createBinOp(spv::Op::OpISub, intType(), stackIdxVal, constPopCount);
         addName(newStackIdxVal, "newStackIdx");

         // *stackIndexVar = stateIdxVal
         createStore(newStackIdxVal, stackIndexVar());
         stackIdxVal = newStackIdxVal;
      }

      // newStateVal = stack[stackIndexVal]
      auto stackPtr = createAccessChain(spv::StorageClass::StorageClassPrivate, stackVar(), { stackIdxVal });
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

   spv::Id readGprSelRef(const latte::GprSelRef &ref)
   {
      switch (ref.sel) {
      case latte::SQ_SEL::SEL_X:
         return readGprChanRef(GprChanRef { ref.gpr, latte::SQ_CHAN::X });
      case latte::SQ_SEL::SEL_Y:
         return readGprChanRef(GprChanRef { ref.gpr, latte::SQ_CHAN::Y });
      case latte::SQ_SEL::SEL_Z:
         return readGprChanRef(GprChanRef { ref.gpr, latte::SQ_CHAN::Z });
      case latte::SQ_SEL::SEL_W:
         return readGprChanRef(GprChanRef { ref.gpr, latte::SQ_CHAN::W });
      case latte::SQ_SEL::SEL_0:
         return makeFloatConstant(0.0f);
      case latte::SQ_SEL::SEL_1:
         return makeFloatConstant(1.0f);
      default:
         decaf_abort("Unexpected SQ_SEL in gpr sel ref");
      }
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

   spv::Id getCbufferRef(const latte::CbufferRef &ref)
   {
      auto thisCbufVar = cbufferVar(ref.bufferId);
      auto cbufIdxVal = getCbufferRefIndex(ref);

      // $ = &CBUFFER{bufferindex}[index]
      auto zeroConst = makeUintConstant(0);
      return createAccessChain(spv::StorageClass::StorageClassUniform, thisCbufVar, { zeroConst, cbufIdxVal });
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
            // We write floats as UINT's which are bitcast to preserve precision
            // in the case that the value will be used as a uint.
            auto srcFloatData = makeUintConstant(source.value.uintValue);
            srcId = createUnaryOp(spv::OpBitcast, floatType(), srcFloatData);
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

      if (srcX.type == SrcVarRef::Type::CBUFFER &&
          srcY.type == SrcVarRef::Type::CBUFFER &&
          srcZ.type == SrcVarRef::Type::CBUFFER &&
          srcW.type == SrcVarRef::Type::CBUFFER &&
          srcX.cbufferChan.chan == latte::SQ_CHAN::X &&
          srcY.cbufferChan.chan == latte::SQ_CHAN::Y &&
          srcZ.cbufferChan.chan == latte::SQ_CHAN::Z &&
          srcW.cbufferChan.chan == latte::SQ_CHAN::W &&
          srcX.cbufferChan.cbuffer.bufferId == srcY.cbufferChan.cbuffer.bufferId &&
          srcY.cbufferChan.cbuffer.bufferId == srcZ.cbufferChan.cbuffer.bufferId &&
          srcZ.cbufferChan.cbuffer.bufferId == srcW.cbufferChan.cbuffer.bufferId &&
          srcX.cbufferChan.cbuffer.index == srcY.cbufferChan.cbuffer.index &&
          srcY.cbufferChan.cbuffer.index == srcZ.cbufferChan.cbuffer.index &&
          srcZ.cbufferChan.cbuffer.index == srcW.cbufferChan.cbuffer.index &&
          srcX.cbufferChan.cbuffer.indexMode == srcY.cbufferChan.cbuffer.indexMode &&
          srcY.cbufferChan.cbuffer.indexMode == srcZ.cbufferChan.cbuffer.indexMode &&
          srcZ.cbufferChan.cbuffer.indexMode == srcW.cbufferChan.cbuffer.indexMode) {
         auto srcCbufferPtr = getCbufferRef(srcX.cbufferChan.cbuffer);
         return createLoad(srcCbufferPtr);
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
      // Perform any neccessary type conversions
      auto srcTypeId = getTypeId(srcId);
      if (srcTypeId == float4Type()) {
         // Nothing to do, we are already a float!
      } else if (srcTypeId == int4Type()) {
         srcId = createUnaryOp(spv::OpBitcast, float4Type(), srcId);
      } else if (srcTypeId == uint4Type()) {
         srcId = createUnaryOp(spv::OpBitcast, float4Type(), srcId);
      } else {
         decaf_abort("Unexpected type at gpr masked instruction write");
      }

      // Grab a reference to our GPR
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

   spv::Id shrinkVector(spv::Id value, uint32_t maxComponents)
   {
      // We only support 4-component vector types here...
      decaf_check(getNumComponents(value) == 4);

      // Figure out what type we need to return
      auto valueType = getTypeId(value);
      auto valueBaseType = this->getContainedTypeId(valueType);

      if (maxComponents == 1) {
         return createOp(spv::OpCompositeExtract, valueBaseType, { value, 0 });
      } else if (maxComponents == 2) {
         return createOp(spv::OpVectorShuffle, vecType(valueBaseType, 2), { value, value, 0, 1 });
      } else if (maxComponents == 3) {
         return createOp(spv::OpVectorShuffle, vecType(valueBaseType, 3), { value, value, 0, 1, 2 });
      } else if (maxComponents == 4) {
         return value;
      } else {
         decaf_abort("Unexpected component count during vector shrink");
      }
   }

   spv::Id applySelMask(spv::Id dest, spv::Id source, std::array<SQ_SEL, 4> mask, uint32_t maxComponents = 4)
   {
      // We only support doing masking against 4-component vectors.
      decaf_check(getNumComponents(source) == 4);

      auto sourceType = getTypeId(source);
      auto sourceBaseType = this->getContainedTypeId(sourceType);

      // For simplicity in the checking below, we set the unused mask values
      // to the defaults that we might expect otherwise.
      for (auto i = maxComponents; i < 4; ++i) {
         mask[i] = static_cast<latte::SQ_SEL>(latte::SQ_SEL::SEL_X + i);
      }

      // If the swizzle is just XYZW, we don't actually need to do anything...
      bool isNoop = true;
      isNoop &= (mask[0] == latte::SQ_SEL::SEL_X);
      isNoop &= (mask[1] == latte::SQ_SEL::SEL_Y);
      isNoop &= (mask[2] == latte::SQ_SEL::SEL_Z);
      isNoop &= (mask[3] == latte::SQ_SEL::SEL_W);
      if (isNoop) {
         return shrinkVector(source, maxComponents);
      }

      // If the swizzle is ____, we should just skip processing entirely
      bool isMasked = true;
      isMasked &= (mask[0] == latte::SQ_SEL::SEL_MASK);
      isMasked &= (mask[1] == latte::SQ_SEL::SEL_MASK);
      isMasked &= (mask[2] == latte::SQ_SEL::SEL_MASK);
      isMasked &= (mask[3] == latte::SQ_SEL::SEL_MASK);
      if (isMasked) {
         decaf_check(dest != spv::NoResult);
         return shrinkVector(dest, maxComponents);
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
         for (auto selIdx = 0u; selIdx < maxComponents; ++selIdx) {
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
            case latte::SQ_SEL::SEL_MASK:
               break;
            default:
               decaf_abort("Unexpected selector during masking operation.");
            }
         }

         if (dest == spv::NoResult) {
            decaf_check(isSwizzleFullyUnmasked(mask));
            dest = source;
         }

         if (maxComponents == 1) {
            return createOp(spv::OpCompositeExtract, sourceBaseType, { source, shuffleIdx[0] });
         } else if (maxComponents == 2) {
            return createOp(spv::Op::OpVectorShuffle, vecType(sourceBaseType, 2), { dest, source, shuffleIdx[0], shuffleIdx[1] });
         } else if (maxComponents == 2) {
            return createOp(spv::Op::OpVectorShuffle, vecType(sourceBaseType, 3), { dest, source, shuffleIdx[0], shuffleIdx[1], shuffleIdx[2] });
         } else if (maxComponents == 4) {
            return createOp(spv::Op::OpVectorShuffle, vecType(sourceBaseType, 4), { dest, source, shuffleIdx[0], shuffleIdx[1], shuffleIdx[2], shuffleIdx[3] });
         } else {
            decaf_abort("Unexpected component count during swizzle");
         }
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

      for (auto selIdx = 0u; selIdx < maxComponents; ++selIdx) {
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

      if (maxComponents == 1) {
         return resultElems[0];
      } else if (maxComponents == 2) {
         return createOp(spv::OpCompositeConstruct, vecType(sourceBaseType, 2), { resultElems[0], resultElems[1] });
      } else if (maxComponents == 2) {
         return createOp(spv::OpCompositeConstruct, vecType(sourceBaseType, 3), { resultElems[0], resultElems[1], resultElems[2] });
      } else if (maxComponents == 4) {
         return createOp(spv::OpCompositeConstruct, vecType(sourceBaseType, 4), { resultElems[0], resultElems[1], resultElems[2], resultElems[3] });
      } else {
         decaf_abort("Unexpected component count during swizzle result construct");
      }
   }

   void writeAluOpDest(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst, spv::Id srcId, bool forAr = false)
   {
      if (inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
         switch (inst.op2.OMOD()) {
         case SQ_ALU_OMOD::D2:
            decaf_check(getTypeId(srcId) == floatType());
            srcId = createBinOp(spv::Op::OpFMul, floatType(), srcId, makeFloatConstant(0.5f));
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

         mARId[unit] = srcId;
      } else {
         mNextPrevResId[unit] = srcId;
      }

      if (inst.word1.ENCODING() != SQ_ALU_ENCODING::OP2 || inst.op2.WRITE_MASK()) {
         // According to the docs, GPR writes are undefined for AR instructions
         // though its worth noting that it does not actually say its illegal.
         decaf_check(!forAr);

         GprChanRef destGpr;
         destGpr.gpr = makeGprRef(inst.word1.DST_GPR(), inst.word1.DST_REL(), inst.word0.INDEX_MODE());
         destGpr.chan = inst.word1.DST_CHAN();
         mAluGroupWrites.push_back({ destGpr, srcId });
      }
   }

   void flushAluGroupWrites()
   {
      for (auto& write : mAluGroupWrites) {
         writeGprChanRef(write.first, write.second);
      }
      mAluGroupWrites.clear();
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

   spv::Id getExportRefVar(const ExportRef& ref, spv::Id dataType)
   {
      if (ref.type == ExportRef::Type::Position) {
         decaf_check(dataType == floatType());
         decaf_check(ref.dataStride == 0);
         decaf_check(ref.arraySize == 1);
         decaf_check(ref.elemCount == 1);
         decaf_check(ref.indexGpr == -1);
         return posExportVar(ref.arrayBase);
      } else if (ref.type == ExportRef::Type::Param) {
         decaf_check(dataType == floatType());
         decaf_check(ref.dataStride == 0);
         decaf_check(ref.arraySize == 1);
         decaf_check(ref.elemCount == 1);
         decaf_check(ref.indexGpr == -1);
         return paramExportVar(ref.arrayBase);
      } else if (ref.type == ExportRef::Type::Pixel) {
         decaf_check(ref.dataStride == 0);
         decaf_check(ref.arraySize == 1);
         decaf_check(ref.elemCount == 1);
         decaf_check(ref.indexGpr == -1);
         return pixelExportVar(ref.arrayBase, dataType);
      } else if (ref.type == ExportRef::Type::ComputedZ) {
         decaf_check(dataType == floatType());
         decaf_check(ref.dataStride == 0);
         decaf_check(ref.arrayBase == 0);
         decaf_check(ref.arraySize == 1);
         decaf_check(ref.elemCount == 1);
         decaf_check(ref.indexGpr == -1);
         return zExportVar();
      } else if (ref.type >= ExportRef::Type::Stream0Write &&
                 ref.type <= ExportRef::Type::Stream3Write) {
         decaf_check(dataType == floatType());
         auto streamIdx =
            static_cast<uint32_t>(ref.type) -
            static_cast<uint32_t>(ExportRef::Type::Stream0Write);
         return memExportWriteVar(streamIdx, ref.dataStride, ref.indexGpr, ref.arrayBase, ref.arraySize, ref.elemCount);
      } else if (ref.type == ExportRef::Type::VsGsRingWrite) {
         decaf_check(dataType == floatType());
         decaf_check(ref.dataStride == 0);
         return vsGsRingExportWriteVar(ref.indexGpr, ref.arrayBase, ref.arraySize, ref.elemCount);
      } else if (ref.type == ExportRef::Type::GsDcRingWrite) {
         decaf_check(dataType == floatType());
         decaf_check(ref.dataStride == 0);
         return gsDcRingExportWriteVar(ref.indexGpr, ref.arrayBase, ref.arraySize, ref.elemCount);
      } else {
         decaf_abort("Encountered unexpected export type");
      }
   }

   // Takes a value as input and expands it out to being a 4-component
   // vector of the same underlying type.
   spv::Id expandVector(spv::Id value)
   {
      auto numComps = getNumComponents(value);

      auto baseType = getTypeId(value);
      if (numComps > 1) {
         baseType = getContainedTypeId(baseType);
      }

      if (baseType == floatType()) {
         auto zeroFConst = makeFloatConstant(0.0f);
         if (numComps == 1) {
            value = createOp(spv::OpCompositeConstruct, float4Type(), { value, zeroFConst, zeroFConst, zeroFConst });
         } else if (numComps == 2) {
            value = createOp(spv::OpCompositeConstruct, float4Type(), { value, zeroFConst, zeroFConst });
         } else if (numComps == 3) {
            value = createOp(spv::OpCompositeConstruct, float4Type(), { value, zeroFConst });
         } else if (numComps == 4) {
            // Already the right size
         } else {
            decaf_abort("Unexpected number of export components.");
         }
      } else if (baseType == intType()) {
         auto zeroConst = makeIntConstant(0);
         if (numComps == 1) {
            value = createOp(spv::OpCompositeConstruct, int4Type(), { value, zeroConst, zeroConst, zeroConst });
         } else if (numComps == 2) {
            value = createOp(spv::OpCompositeConstruct, int4Type(), { value, zeroConst, zeroConst });
         } else if (numComps == 3) {
            value = createOp(spv::OpCompositeConstruct, int4Type(), { value, zeroConst });
         } else if (numComps == 4) {
            // Already the right size
         } else {
            decaf_abort("Unexpected number of export components.");
         }
      } else if (baseType == uintType()) {
         auto zeroUConst = makeUintConstant(0);
         if (numComps == 1) {
            value = createOp(spv::OpCompositeConstruct, uint4Type(), { value, zeroUConst, zeroUConst, zeroUConst });
         } else if (numComps == 2) {
            value = createOp(spv::OpCompositeConstruct, uint4Type(), { value, zeroUConst, zeroUConst });
         } else if (numComps == 3) {
            value = createOp(spv::OpCompositeConstruct, uint4Type(), { value, zeroUConst });
         } else if (numComps == 4) {
            // Already the right size
         } else {
            decaf_abort("Unexpected number of export components.");
         }
      } else {
         decaf_abort("Unexpected export source data type");
      }

      return value;
   }

   void writeExportRef(const ExportMaskRef& ref, spv::Id srcId)
   {
      // Fetch the underlying type of the export.  Note that we require that
      // this method be invoked with 4-component vectors only!
      auto srcBaseType = getContainedTypeId(getTypeId(srcId));

      // Find the export reference for this, we pass in the source type to hint
      // at the export creation what the output type should be.  This function
      // must return something that is of the same type!
      auto exportPtr = getExportRefVar(ref.output, srcBaseType);

      // Lets figure out the correct size of the data so that we can
      // shrink the source down to the right size at the same time.
      auto exportType = getDerefTypeId(exportPtr);
      auto numExportComps = getNumTypeComponents(exportType);

      // Apply any export masking operation thats needed, this will also
      // shrink the object down to the expected size at the same time.
      // Note: We shouldn't be reading from outputs, but sometimes they
      // mask data into a write, which makes very little sense...
      auto origData = createLoad(exportPtr);
      auto exportVal = applySelMask(origData, srcId, ref.mask, numExportComps);

      auto sourceValType = getTypeId(exportVal);

      // Lets perform any specialized behaviour depending on the export type
      if (ref.output.type == ExportRef::Type::Position) {
         decaf_check(sourceValType == float4Type());
         // Need to reposition the depth values from (-1.0 to 1.0) to (0.0 to 1.0)

         auto zeroConst = makeUintConstant(0);
         auto oneConst = makeUintConstant(1);
         auto zeroFConst = makeFloatConstant(0.0f);
         auto oneFConst = makeFloatConstant(1.0f);

         auto posMulAddPtr = createAccessChain(spv::StorageClass::StorageClassPushConstant, vsPushConstVar(), { zeroConst });
         auto posMulAddVal = createLoad(posMulAddPtr);

         auto zSpaceMulPtr = createAccessChain(spv::StorageClass::StorageClassPushConstant, vsPushConstVar(), { oneConst });
         auto zSpaceMulVal = createLoad(zSpaceMulPtr);

         // pos.xy = (pos.xy * posMulAdd.xy) + posMulAdd.zw;

         auto posMulPart = createOp(spv::OpVectorShuffle, float2Type(), { posMulAddVal, posMulAddVal, 0, 1 });
         auto posMulVal = createOp(spv::OpCompositeConstruct, float4Type(), { posMulPart, oneFConst, oneFConst });

         auto posAddPart = createOp(spv::OpVectorShuffle, float2Type(), { posMulAddVal, posMulAddVal, 2, 3 });
         auto posAddVal = createOp(spv::OpCompositeConstruct, float4Type(), { posAddPart, zeroFConst, zeroFConst });

         exportVal = createBinOp(spv::OpFMul, float4Type(), exportVal, posMulVal);
         exportVal = createBinOp(spv::OpFAdd, float4Type(), exportVal, posAddVal);

         // pos.y = -pos.y

         auto exportY = createOp(spv::OpCompositeExtract, floatType(), { exportVal, 1 });
         exportY = createUnaryOp(spv::OpFNegate, floatType(), exportY);
         exportVal = createOp(spv::OpCompositeInsert, float4Type(), { exportY, exportVal, 1 });

         // pos.z = (pos.z + (pos.w * zSpaceMul.x)) * zSpaceMul.y;

         auto zsYWMul = createOp(spv::OpCompositeExtract, floatType(), { zSpaceMulVal, 0 });
         auto zsYYMul = createOp(spv::OpCompositeExtract, floatType(), { zSpaceMulVal, 1 });

         auto exportZ = createOp(spv::OpCompositeExtract, floatType(), { exportVal, 2 });
         auto exportW = createOp(spv::OpCompositeExtract, floatType(), { exportVal, 3 });

         auto yWAdd = createBinOp(spv::OpFMul, floatType(), exportW, zsYWMul);
         auto zAdj = createBinOp(spv::OpFAdd, floatType(), exportZ, yWAdd);
         auto zFinal = createBinOp(spv::OpFMul, floatType(), zAdj, zsYYMul);

         exportVal = createOp(spv::OpCompositeInsert, float4Type(), { zFinal, exportVal, 2 });
      }

      if (ref.output.type == ExportRef::Type::Pixel) {
         decaf_check(sourceValType == float4Type() || sourceValType == int4Type() || sourceValType == uint4Type());

         auto zeroConst = makeUintConstant(0);
         auto oneConst = makeUintConstant(1);

         // We use the first exported pixel to perform alpha reference testing.  This
         // may not actually be the correct behaviour.
         // TODO: Check which exported pixel does alpha testing.
         if (ref.output.arrayBase == 0 && sourceValType == float4Type()) {
            auto exportAlpha = createOp(spv::OpCompositeExtract, floatType(), { exportVal, 3 });

            auto alphaFuncPtr = createAccessChain(spv::StorageClassPushConstant, psPushConstVar(), { zeroConst });
            auto alphaDataVal = createLoad(alphaFuncPtr);
            auto alphaFuncVal = createBinOp(spv::OpBitwiseAnd, uintType(), alphaDataVal, makeUintConstant(0xFF));

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
            auto makeEqCompareBlock = [&](bool wantEquality)
            {
               auto epsilonConst = makeFloatConstant(0.0001f);
               auto alphaDiff = createBinOp(spv::OpFSub, floatType(), exportAlpha, alphaRefVal);
               auto alphaDiffAbs = createBuiltinCall(floatType(), glslStd450(), GLSLstd450::GLSLstd450FAbs, { alphaDiff });
               spv::Id pred;
               if (wantEquality) {
                  pred = createBinOp(spv::OpFOrdGreaterThan, boolType(), alphaDiffAbs, epsilonConst);
               } else {
                  pred = createBinOp(spv::OpFOrdLessThanEqual, boolType(), alphaDiffAbs, epsilonConst);
               }
               auto block = spv::Builder::If { pred, spv::SelectionControlMaskNone, *this };
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
            makeEqCompareBlock(true);
            addSwitchBreak();

            nextSwitchSegment(switchSegments, latte::REF_FUNC::LESS_EQUAL);
            makeCompareBlock(spv::OpFOrdLessThanEqual);
            addSwitchBreak();

            nextSwitchSegment(switchSegments, latte::REF_FUNC::GREATER);
            makeCompareBlock(spv::OpFOrdGreaterThan);
            addSwitchBreak();

            nextSwitchSegment(switchSegments, latte::REF_FUNC::NOT_EQUAL);
            makeEqCompareBlock(false);
            addSwitchBreak();

            nextSwitchSegment(switchSegments, latte::REF_FUNC::GREATER_EQUAL);
            makeCompareBlock(spv::OpFOrdGreaterThanEqual);
            addSwitchBreak();

            endSwitch(switchSegments);
         }

         auto pixelTmpVar = createVariable(spv::StorageClass::StorageClassPrivate, sourceValType, "_pixelTmp");
         createStore(exportVal, pixelTmpVar);

         auto alphaDataPtr = createAccessChain(spv::StorageClassPushConstant, psPushConstVar(), { zeroConst });
         auto alphaDataVal = createLoad(alphaDataPtr);
         auto logicOpVal = createBinOp(spv::OpShiftRightLogical, uintType(), alphaDataVal, makeUintConstant(8));

         auto lopSet = createBinOp(spv::Op::OpIEqual, boolType(), logicOpVal, makeUintConstant(1));
         auto block = spv::Builder::If { lopSet, spv::SelectionControlMaskNone, *this };
         {
            if (sourceValType == float4Type()) {
               auto newValElem = makeFloatConstant(1.0f);
               auto newVal = makeCompositeConstant(float4Type(), { newValElem, newValElem, newValElem, newValElem });
               createStore(newVal, pixelTmpVar);
            } else if (sourceValType == int4Type()) {
               auto newValElem = makeIntConstant(0xFFFFFFFF);
               auto newVal = makeCompositeConstant(int4Type(), { newValElem, newValElem, newValElem, newValElem });
               createStore(newVal, pixelTmpVar);
            } else if (sourceValType == uint4Type()) {
               auto newValElem = makeUintConstant(0xFFFFFFFF);
               auto newVal = makeCompositeConstant(uint4Type(), { newValElem, newValElem, newValElem, newValElem });
               createStore(newVal, pixelTmpVar);
            } else {
               decaf_abort("Unexpected source pixel variable type");
            }
         }
         block.makeBeginElse();
         {
            auto lopClear = createBinOp(spv::Op::OpIEqual, boolType(), logicOpVal, makeUintConstant(2));
            auto block = spv::Builder::If { lopClear, spv::SelectionControlMaskNone, *this };
            {
               if (sourceValType == float4Type()) {
                  auto newValElem = makeFloatConstant(0.0f);
                  auto newVal = makeCompositeConstant(float4Type(), { newValElem, newValElem, newValElem, newValElem });
                  createStore(newVal, pixelTmpVar);
               } else if (sourceValType == int4Type()) {
                  auto newValElem = makeIntConstant(0);
                  auto newVal = makeCompositeConstant(int4Type(), { newValElem, newValElem, newValElem, newValElem });
                  createStore(newVal, pixelTmpVar);
               } else if (sourceValType == uint4Type()) {
                  auto newValElem = makeUintConstant(0);
                  auto newVal = makeCompositeConstant(uint4Type(), { newValElem, newValElem, newValElem, newValElem });
                  createStore(newVal, pixelTmpVar);
               } else {
                  decaf_abort("Unexpected source pixel variable type");
               }
            }
            block.makeEndIf();
         }
         block.makeEndIf();

         // We need to premultiply the alpha in cases where premultiplied alpha is enabled
         // globally but this specific target is not performing the premultiplication.
         if (sourceValType == float4Type()) {
            auto zeroConst = makeUintConstant(0);
            auto oneConst = makeUintConstant(1);
            auto twoConst = makeUintConstant(2);
            auto oneFConst = makeFloatConstant(1.0f);

            auto needsPremulPtr = createAccessChain(spv::StorageClassPushConstant, psPushConstVar(), { twoConst });
            auto needsPremulVal = createLoad(needsPremulPtr);

            auto targetBitConst = makeUintConstant(1 << ref.output.arrayBase);
            auto targetBitVal = createBinOp(spv::OpBitwiseAnd, uintType(), needsPremulVal, targetBitConst);
            auto pred = createBinOp(spv::OpINotEqual, boolType(), targetBitVal, zeroConst);
            auto block = spv::Builder::If { pred, spv::SelectionControlMaskNone, *this };
            {
               exportVal = createLoad(pixelTmpVar);

               auto exportAlpha = createOp(spv::OpCompositeExtract, floatType(), { exportVal, 3 });
               auto premulMul = createOp(spv::OpCompositeConstruct, float4Type(), { exportAlpha, exportAlpha, exportAlpha, oneFConst });
               auto premulExportVal = createBinOp(spv::OpFMul, float4Type(), exportVal, premulMul);

               createStore(premulExportVal, pixelTmpVar);
            }
            block.makeEndIf();
         }

         exportVal = createLoad(pixelTmpVar);
      }

      createStore(exportVal, exportPtr);
   }

   spv::Id vertexIdVar()
   {
      if (!mVertexId) {
         mVertexId = createVariable(spv::StorageClassInput, intType(), "VertexID");
         addDecoration(mVertexId, spv::DecorationBuiltIn, spv::BuiltInVertexIndex);
         mEntryPoint->addIdOperand(mVertexId);
      }
      return mVertexId;
   }

   spv::Id instanceIdVar()
   {
      if (!mInstanceId) {
         mInstanceId = createVariable(spv::StorageClassInput, intType(), "InstanceID");
         addDecoration(mInstanceId, spv::DecorationBuiltIn, spv::BuiltInInstanceIndex);
         mEntryPoint->addIdOperand(mInstanceId);
      }
      return mInstanceId;
   }

   spv::Id fragCoordVar()
   {
      if (!mFragCoord) {
         mFragCoord = createVariable(spv::StorageClassInput, float4Type(), "FragCoord");
         addDecoration(mFragCoord, spv::DecorationBuiltIn, spv::BuiltInFragCoord);
         mEntryPoint->addIdOperand(mFragCoord);
      }
      return mFragCoord;
   }

   spv::Id frontFacingVar()
   {
      if (!mFrontFacing) {
         mFrontFacing = createVariable(spv::StorageClassInput, boolType(), "FrontFacing");
         addDecoration(mFrontFacing, spv::DecorationBuiltIn, spv::BuiltInFrontFacing);
         mEntryPoint->addIdOperand(mFrontFacing);
      }
      return mFrontFacing;
   }

   spv::Id layerIdVar()
   {
      if (!mLayerId) {
         mLayerId = createVariable(spv::StorageClassOutput, intType(), "LayerID");
         addDecoration(mLayerId, spv::DecorationBuiltIn, spv::BuiltInLayer);
         mEntryPoint->addIdOperand(mLayerId);
      }
      return mLayerId;
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

   spv::Id inputRingVar(int index)
   {
      while (mRingInputs.size() <= index) {
         mRingInputs.push_back(spv::NoResult);
      }

      auto inputVar = mRingInputs[index];
      if (!inputVar) {
         inputVar = createVariable(spv::StorageClassInput, arrayType(float4Type(), 3),
                                        fmt::format("RINGIN_{}", index).c_str());
         addDecoration(inputVar, spv::DecorationLocation, static_cast<int>(index));

         mEntryPoint->addIdOperand(inputVar);
         mRingInputs[index] = inputVar;
      }

      return inputVar;
   }

   spv::Id vsPushConstVar()
   {
      if (!mVsPushConsts) {
         auto vsPushStruct = makeStructType({ float4Type(), float4Type() }, "VS_PUSH_CONSTANTS");
         addMemberDecoration(vsPushStruct, 0, spv::DecorationOffset, 0);
         addMemberName(vsPushStruct, 0, "posMulAdd");
         addMemberDecoration(vsPushStruct, 1, spv::DecorationOffset, 16);
         addMemberName(vsPushStruct, 1, "zSpaceMul");
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
         auto psPushStruct = makeStructType({ uintType(), floatType(), uintType() }, "PS_PUSH_DATA");
         addMemberDecoration(psPushStruct, 0, spv::DecorationOffset, 32 + 0);
         addMemberName(psPushStruct, 0, "alphaFunc");
         addMemberDecoration(psPushStruct, 1, spv::DecorationOffset, 32 + 4);
         addMemberName(psPushStruct, 1, "alphaRef");
         addMemberDecoration(psPushStruct, 2, spv::DecorationOffset, 32 + 8);
         addMemberName(psPushStruct, 2, "needsPremultiply");
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

         auto bindingIdx = mBindingBase + latte::MaxTextures;

         mRegistersBuffer = createVariable(spv::StorageClassUniform, structType, "CFILE");
         addDecoration(mRegistersBuffer, spv::DecorationDescriptorSet, 0);
         addDecoration(mRegistersBuffer, spv::DecorationBinding, bindingIdx);
      }

      return mRegistersBuffer;
   }

   spv::Id cbufferVar(uint32_t cbufferIdx)
   {
      decaf_check(cbufferIdx < latte::MaxUniformBlocks);

      auto cbuffer = mUniformBuffers[cbufferIdx];
      if (!cbuffer) {
         auto valuesType = arrayType(float4Type(), 0);
         addDecoration(valuesType, spv::DecorationArrayStride, 16);

         auto structType = this->makeStructType({ valuesType }, fmt::format("CBUFFER_DATA_{}", cbufferIdx).c_str());
         addMemberDecoration(structType, 0, spv::DecorationOffset, 0);
         addDecoration(structType, spv::DecorationBlock);
         addMemberName(structType, 0, "values");

         auto bindingIdx = mBindingBase + latte::MaxTextures + cbufferIdx;

         cbuffer = createVariable(spv::StorageClassUniform, structType, fmt::format("CBUFFER_{}", cbufferIdx).c_str());
         addDecoration(cbuffer, spv::DecorationDescriptorSet, 0);
         addDecoration(cbuffer, spv::DecorationBinding, bindingIdx);

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

         auto bindingIdx = mBindingBase + samplerIdx;

         addDecoration(samplerId, spv::DecorationDescriptorSet, 0);
         addDecoration(samplerId, spv::DecorationBinding, bindingIdx);

         mSamplers[samplerIdx] = samplerId;
      }

      return samplerId;
   }

   spv::Id textureVarType(uint32_t textureIdx, latte::SQ_TEX_DIM texDim, TextureInputType texFormat)
   {
      decaf_check(textureIdx < latte::MaxTextures);

      spv::Id resultType;
      if (texFormat == TextureInputType::FLOAT) {
         resultType = floatType();
      } else if (texFormat == TextureInputType::INT) {
         resultType = uintType();
      } else {
         decaf_abort("Unexpected texture input format");
      }

      auto textureType = mTextureTypes[textureIdx];
      if (!textureType) {
         // TODO: This shouldn't exist here...
         switch (texDim) {
         case latte::SQ_TEX_DIM::DIM_1D:
            textureType = makeImageType(resultType, spv::Dim1D, false, false, false, 1, spv::ImageFormatUnknown);
            break;
         case latte::SQ_TEX_DIM::DIM_2D:
         case latte::SQ_TEX_DIM::DIM_2D_MSAA:
            textureType = makeImageType(resultType, spv::Dim2D, false, false, false, 1, spv::ImageFormatUnknown);
            break;
         case latte::SQ_TEX_DIM::DIM_3D:
            textureType = makeImageType(resultType, spv::Dim3D, false, false, false, 1, spv::ImageFormatUnknown);
            break;
         case latte::SQ_TEX_DIM::DIM_CUBEMAP:
            textureType = makeImageType(resultType, spv::Dim2D, false, true, false, 1, spv::ImageFormatUnknown);
            break;
         case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
            textureType = makeImageType(resultType, spv::Dim1D, false, true, false, 1, spv::ImageFormatUnknown);
            break;
         case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
         case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
            textureType = makeImageType(resultType, spv::Dim2D, false, true, false, 1, spv::ImageFormatUnknown);
            break;
         default:
            decaf_abort("Unexpected texture dim type");
         }

         mTextureTypes[textureIdx] = textureType;
      }

      return textureType;
   }

   spv::Id textureVar(uint32_t textureIdx, latte::SQ_TEX_DIM texDim, TextureInputType texFormat)
   {
      decaf_check(textureIdx < latte::MaxTextures);

      auto textureId = mTextures[textureIdx];
      if (!textureId) {
         textureId = createVariable(spv::StorageClassUniformConstant, textureVarType(textureIdx, texDim, texFormat));
         addName(textureId, fmt::format("TEXTURE_{}", textureIdx).c_str());

         auto bindingIdx = mBindingBase + textureIdx;

         addDecoration(textureId, spv::DecorationDescriptorSet, 0);
         addDecoration(textureId, spv::DecorationBinding, bindingIdx);

         mTextures[textureIdx] = textureId;
      }

      return textureId;
   }

   spv::Id pixelExportVar(uint32_t pixelIdx, spv::Id outputType)
   {
      decaf_check(pixelIdx < latte::MaxRenderTargets);

      auto exportType = vecType(outputType, 4);

      auto& exportId = mPixelExports[pixelIdx];
      if (!exportId) {
         exportId = createVariable(spv::StorageClass::StorageClassOutput, exportType, fmt::format("PIXEL_{}", pixelIdx).c_str());
         addDecoration(exportId, spv::Decoration::DecorationLocation, pixelIdx);
         mPixelExports[pixelIdx] = exportId;

         mEntryPoint->addIdOperand(exportId);
      }

      // Lets confirm that the type is correct!  If we fetched this from the
      // map, its possible that the type has changed, which we do not handle.
      decaf_check(getDerefTypeId(exportId) == exportType);

      return exportId;
   }

   spv::Id posExportVar(uint32_t posIdx)
   {
      decaf_check(posIdx < 4);

      auto& exportId = mPosExports[posIdx];
      if (!exportId) {
         exportId = createVariable(spv::StorageClass::StorageClassOutput, float4Type(), fmt::format("POS_{}", posIdx).c_str());

         if (posIdx == 0) {
            // Mark this as being the position output
            addDecoration(exportId, spv::DecorationBuiltIn, spv::BuiltInPosition);
         } else {
            // Mark this to a location, not sure if this is really needed
            addDecoration(exportId, spv::Decoration::DecorationLocation, 60 + posIdx);
         }

         mEntryPoint->addIdOperand(exportId);
      }

      return exportId;
   }

   spv::Id paramExportVar(uint32_t paramIdx)
   {
      decaf_check(paramIdx < 32);

      auto& exportId = mParamExports[paramIdx];
      if (!exportId) {
         exportId = createVariable(spv::StorageClass::StorageClassOutput, float4Type(), fmt::format("PARAM_{}", paramIdx).c_str());
         addDecoration(exportId, spv::Decoration::DecorationLocation, paramIdx);

         mEntryPoint->addIdOperand(exportId);
      }

      return exportId;
   }

   spv::Id zExportVar()
   {
      if (!mZExport) {
         addExecutionMode(mFunctions["main"], spv::ExecutionModeDepthReplacing);

         mZExport = createVariable(spv::StorageClass::StorageClassOutput, floatType(), "Z_EXPORT");
         addDecoration(mZExport, spv::DecorationBuiltIn, spv::BuiltInFragDepth);
         mEntryPoint->addIdOperand(mZExport);
      }

      return mZExport;
   }

   spv::Id memExportWriteVar(uint32_t streamIdx, uint32_t dataStride, uint32_t indexGpr, uint32_t arrayBase, uint32_t arraySize, uint32_t elemCount)
   {
      decaf_check(streamIdx < 4);
      decaf_check(dataStride > 0);

      // We do not currently support dynamic indexing into streamout
      decaf_check(indexGpr == -1);

      // TODO: Support write prevention based on the array size.

      // We have to add the transform feedback capability to use Xfb
      addCapability(spv::CapabilityTransformFeedback);
      addExecutionMode(mFunctions["main"], spv::ExecutionModeXfb);

      // Calculate the offset of vec4's since that is all we currently support
      auto& streamExports = mMemWriteExports[streamIdx];

      // We only support writing 1-4 dwords per memory write.  This might break
      // down when someone does a burst write :S
      decaf_check(elemCount == 1);
      decaf_check(arraySize <= 4);
      auto writeElemCount = arraySize * elemCount;

      // We only support writing in vec4 increments
      auto bufferOffset = arrayBase * 4;

      // Determine the type of this stream out object
      spv::Id resultType = spv::NoResult;
      if (writeElemCount == 1) {
         resultType = floatType();
      } else if (writeElemCount == 2) {
         resultType = float2Type();
      } else if (writeElemCount == 3) {
         resultType = float3Type();
      } else if (writeElemCount == 4) {
         resultType = float4Type();
      } else {
         decaf_abort("Unexpected stream out item size");
      }

      auto& exportId = streamExports[bufferOffset];
      if (!exportId) {
         // We currently place the stream-out data at the end of the params
         // assuming that nobody will ever use all stream-out with parameter
         // output as well.  It may break in wierd cases.

         auto streamOutIndex = mNumStreamOut++;

         exportId = createVariable(spv::StorageClassOutput, resultType, fmt::format("STREAMOUT_{}_{}", streamIdx, bufferOffset).c_str());
         addDecoration(exportId, spv::DecorationLocation, 31 - streamOutIndex);
         addDecoration(exportId, spv::DecorationXfbBuffer, streamIdx);
         addDecoration(exportId, spv::DecorationXfbStride, dataStride);
         addDecoration(exportId, spv::DecorationOffset, bufferOffset);

         mEntryPoint->addIdOperand(exportId);
      }

      decaf_check(getDerefTypeId(exportId) == resultType);

      return exportId;
   }

   spv::Id vsGsRingExportWriteVar(uint32_t indexGpr, uint32_t arrayBase, uint32_t arraySize, uint32_t elemCount)
   {
      // TODO: Support write prevention based on the array size.

      // We do not currently support dynamic indexing into streamout
      decaf_check(indexGpr == -1);

      auto bufferOffset = arrayBase * elemCount;
      decaf_check(bufferOffset % 16 == 0);
      auto bufferVec4Offset = bufferOffset / 16;

      // Determine the size for this ringbuffer write
      spv::Id resultType = spv::NoResult;
      if (elemCount == 1) {
         resultType = floatType();
      } else if (elemCount == 2) {
         resultType = float2Type();
      } else if (elemCount == 3) {
         resultType = float3Type();
      } else if (elemCount == 4) {
         resultType = float4Type();
      } else {
         decaf_abort("Unexpected element count for ringbuffer write");
      }

      auto& exportId = mRingWriteExports[bufferVec4Offset];
      if (!exportId) {
         exportId = createVariable(spv::StorageClassOutput, resultType, fmt::format("VSRING_{}", bufferVec4Offset).c_str());
         addDecoration(exportId, spv::DecorationLocation, bufferVec4Offset);

         mEntryPoint->addIdOperand(exportId);
      }

      decaf_check(getDerefTypeId(exportId) == resultType);

      return exportId;
   }

   spv::Id gsDcRingExportWriteVar(uint32_t indexGpr, uint32_t arrayBase, uint32_t arraySize, uint32_t elemCount)
   {
      // TODO: Support write prevention based on the array size.

      // Calculate the overall offset into the buffer
      auto bufferOffset = arrayBase * elemCount;

      // Grab the vec4 offset, writes must only occur on the boundaries
      // of a single vec4 write...
      decaf_check(bufferOffset % 16 == 0);
      auto ringVec4Offset = bufferOffset / 16;

      // Create a pointer to our memory object
      auto ringOffset = makeIntConstant(ringVec4Offset);

      if (indexGpr != -1) {
         // Fetch the index GPR value
         GprSelRef gprRef;
         gprRef.gpr.number = indexGpr;
         gprRef.gpr.indexMode = GprIndexMode::None;
         gprRef.sel = latte::SQ_SEL::SEL_X;
         auto indexGprVal = readGprSelRef(gprRef);

         // Bitcast it to an integer
         indexGprVal = createUnaryOp(spv::OpBitcast, intType(), indexGprVal);

         // Add the indexed value to the ring offset
         ringOffset = createBinOp(spv::OpIAdd, intType(), indexGprVal, ringOffset);
      }

      return createAccessChain(spv::StorageClassPrivate, ringVar(), { ringOffset });
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

   spv::Id ringVar()
   {
      if (!mRing) {
         auto ringType = makeArrayType(float4Type(), makeUintConstant(128), 0);
         mRing = createVariable(spv::StorageClass::StorageClassPrivate, ringType, "LocalRing");
      }
      return mRing;
   }

   spv::Id ringOffsetVar()
   {
      if (!mRingOffset) {
         mRingOffset = createVariable(spv::StorageClass::StorageClassPrivate, uintType(), "RingIndex");
      }
      return mRingOffset;
   }

   spv::Id ALVar()
   {
      if (!mAL) {
         mAL = createVariable(spv::StorageClass::StorageClassPrivate, intType(), "ALVar");
      }
      return mAL;
   }

   void resetAr()
   {
      // It is not legal to modify the AR register in the same instruction group
      // that reads it, so we don't need to worry about swapping things around
      // like we do for PV/PS.

      for (auto i = 0; i < 4; ++i) {
         mARId[i] = spv::NoResult;
      }
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

      auto arVal = mARId[unit];
      decaf_check(getTypeId(arVal) == intType());
      return arVal;
   }

   bool hasFunction(const std::string& name) const
   {
      return mFunctions.find(name) != mFunctions.end();
   }

   spv::Function *getFunction(const std::string& name)
   {
      auto &func = mFunctions[name];
      if (!func) {
         auto savePos = getBuildPoint();
         auto entryBlock = static_cast<spv::Block *>(nullptr);
         func = makeFunctionEntry(spv::NoPrecision, makeVoidType(),
                                  name.c_str(), {}, {}, &entryBlock);
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

   bool isStreamOutUsed(uint32_t bufferIdx) const
   {
      decaf_check(bufferIdx <= latte::MaxStreamOutBuffers);
      return !mMemWriteExports[bufferIdx].empty();
   }

   bool isPixelOutUsed(uint32_t exportIdx) const
   {
      return mPixelExports.find(exportIdx) != mPixelExports.end();
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
   std::vector<std::pair<GprChanRef, spv::Id>> mAluGroupWrites;

   uint32_t mBindingBase;
   spv::Instruction *mEntryPoint = nullptr;
   std::unordered_map<std::string, spv::Function*> mFunctions;

   spv::Id mVertexId = spv::NoResult;
   spv::Id mInstanceId = spv::NoResult;
   spv::Id mFragCoord = spv::NoResult;
   spv::Id mFrontFacing = spv::NoResult;
   spv::Id mLayerId = spv::NoResult;

   spv::Id mRegistersBuffer = spv::NoResult;
   std::array<spv::Id, latte::MaxUniformBlocks> mUniformBuffers = { spv::NoResult };

   spv::Id mVsPushConsts = spv::NoResult;
   spv::Id mGsPushConsts = spv::NoResult;
   spv::Id mPsPushConsts = spv::NoResult;

   std::vector<spv::Id> mAttribInputs;
   std::vector<spv::Id> mParamInputs;
   std::vector<spv::Id> mRingInputs;

   std::array<spv::Id, latte::MaxSamplers> mSamplers = { spv::NoResult };
   std::array<spv::Id, latte::MaxTextures> mTextureTypes = { spv::NoResult };
   std::array<spv::Id, latte::MaxTextures> mTextures = { spv::NoResult };

   std::map<uint32_t, spv::Id> mPixelExports;
   std::map<uint32_t, spv::Id> mPosExports;
   std::map<uint32_t, spv::Id> mParamExports;
   spv::Id mZExport = spv::NoResult;
   std::array<std::map<uint32_t, spv::Id>, latte::MaxStreamOutBuffers> mMemWriteExports;
   uint32_t mNumStreamOut = 0;
   std::map<uint32_t, spv::Id> mRingWriteExports;

   spv::Id mState = spv::NoResult;
   spv::Id mPredicate = spv::NoResult;
   spv::Id mStackIndex = spv::NoResult;
   spv::Id mStack = spv::NoResult;
   spv::Id mGpr = spv::NoResult;
   spv::Id mAL = spv::NoResult;
   spv::Id mRing = spv::NoResult;
   spv::Id mRingOffset = spv::NoResult;

   std::array<spv::Id, 4> mARId = { spv::NoResult };
   std::array<spv::Id, 5> mPrevResId = { spv::NoResult };
   std::array<spv::Id, 5> mNextPrevResId = { spv::NoResult };

};

} //namespace spirv

#endif // ifdef DECAF_VULKAN
