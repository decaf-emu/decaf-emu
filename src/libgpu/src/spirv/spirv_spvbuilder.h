#pragma once
#ifdef DECAF_VULKAN
#include <SpvBuilder.h>
#include <GLSL.std.450.h>

namespace spirv
{

class SpvBuilder : public spv::Builder
{
public:
   SpvBuilder()
      : spv::Builder(0x10000, 0xFFFFFFFF, nullptr)
   {
   }

   // ------------------------------------------------------------
   // Constants
   // ------------------------------------------------------------

   spv::Id makeFloatConstant(float f, bool specConstant = false)
   {
      if (specConstant) {
         return spv::Builder::makeFloatConstant(f, specConstant);
      }

      auto constVal = mFloatConstants[f];
      if (constVal) {
         return constVal;
      }

      constVal = spv::Builder::makeFloatConstant(f, specConstant);
      addName(constVal, fmt::format("CONST_{}f", f).c_str());

      mFloatConstants[f] = constVal;
      return constVal;
   }

   spv::Id makeUintConstant(unsigned int u, bool specConstant = false)
   {
      if (specConstant) {
         return spv::Builder::makeUintConstant(u, specConstant);
      }

      auto constVal = mUintConstants[u];
      if (constVal) {
         return constVal;
      }

      constVal = spv::Builder::makeUintConstant(u, specConstant);
      switch (u) {
      case 0: addName(constVal, "CONST_0_or_X"); break;
      case 1: addName(constVal, "CONST_1_or_Y"); break;
      case 2: addName(constVal, "CONST_2_or_Z"); break;
      case 3: addName(constVal, "CONST_3_or_W"); break;
      case 4: addName(constVal, "CONST_4_or_T"); break;
      default:
         if (u < 100) {
            addName(constVal, fmt::format("CONST_{}", u).c_str());
         } else {
            addName(constVal, fmt::format("CONST_0x{:x}", u).c_str());
         }
         break;
      }

      mUintConstants[u] = constVal;
      return constVal;
   }

   spv::Id vectorizeConstant(spv::Id source, int elemCount)
   {
      std::vector<spv::Id> sources(elemCount);
      for (auto i = 0u; i < sources.size(); ++i) {
         sources[i] = source;
      }
      return makeCompositeConstant(makeVectorType(getTypeId(source), elemCount), sources);
   }


   // ------------------------------------------------------------
   // Types
   // ------------------------------------------------------------

   spv::Id makeIntType(int width)
   {
      if (width == 8) {
         addCapability(spv::Capability::CapabilityInt8);
      } else if (width == 16) {
         addCapability(spv::Capability::CapabilityInt16);
      }
      return spv::Builder::makeIntType(width);
   }

   spv::Id makeUintType(int width)
   {
      if (width == 8) {
         addCapability(spv::Capability::CapabilityInt8);
      } else if (width == 16) {
         addCapability(spv::Capability::CapabilityInt16);
      }
      return spv::Builder::makeUintType(width);
   }

   spv::Id samplerType()
   {
      if (!mSamplerType) {
         mSamplerType = makeSamplerType();
         addName(mSamplerType, "sampler");
      }
      return mSamplerType;
   }

   spv::Id boolType()
   {
      if (!mBoolType) {
         mBoolType = makeBoolType();
         addName(mBoolType, "bool");
      }
      return mBoolType;
   }

   spv::Id floatType()
   {
      if (!mFloatType) {
         mFloatType = makeFloatType(32);
         addName(mFloatType, "float");
      }
      return mFloatType;
   }
   spv::Id float2Type() { return vecType(floatType(), 2); }
   spv::Id float3Type() { return vecType(floatType(), 3); }
   spv::Id float4Type() { return vecType(floatType(), 4); }

   spv::Id ubyteType()
   {
      if (!mUbyteType) {
         mUbyteType = makeUintType(8);
         addName(mUbyteType, "ubyte");
      }
      return mUbyteType;
   }
   spv::Id ubyte2Type() { return vecType(ubyteType(), 2); }
   spv::Id ubyte3Type() { return vecType(ubyteType(), 3); }
   spv::Id ubyte4Type() { return vecType(ubyteType(), 4); }

   spv::Id ushortType()
   {
      if (!mUshortType) {
         mUshortType = makeUintType(16);
         addName(mUshortType, "ushort");
      }
      return mUshortType;
   }
   spv::Id ushort2Type() { return vecType(ushortType(), 2); }
   spv::Id ushort3Type() { return vecType(ushortType(), 3); }
   spv::Id ushort4Type() { return vecType(ushortType(), 4); }

   spv::Id intType()
   {
      if (!mIntType) {
         mIntType = makeIntType(32);
         addName(mIntType, "int");
      }
      return mIntType;
   }
   spv::Id int2Type() { return vecType(intType(), 2); }
   spv::Id int3Type() { return vecType(intType(), 3); }
   spv::Id int4Type() { return vecType(intType(), 4); }

   spv::Id uintType()
   {
      if (!mUintType) {
         mUintType = makeUintType(32);
         addName(mUintType, "uint");
      }
      return mUintType;
   }
   spv::Id uint2Type() { return vecType(uintType(), 2); }
   spv::Id uint3Type() { return vecType(uintType(), 3); }
   spv::Id uint4Type() { return vecType(uintType(), 4); }

   spv::Id vecType(spv::Id elemType, int elemCount)
   {
      if (elemCount == 1) {
         return elemType;
      }

      auto vecPair = std::make_pair(elemType, elemCount);
      auto vecType = mVecType[vecPair];
      if (!vecType) {
         vecType = makeVectorType(elemType, elemCount);

         auto baseTypeName = getTypeName(elemType);
         if (baseTypeName.empty()) {
            decaf_abort("Unexpected element type for vector type");
         }
         addName(vecType, fmt::format("{}{}", baseTypeName, elemCount).c_str());

         mVecType[vecPair] = vecType;
      }
      return vecType;
   }

   spv::Id arrayType(spv::Id elemType, int elemCount = 0)
   {
      auto arrPair = std::make_pair(elemType, elemCount);
      auto arrType = mArrType[arrPair];
      if (!arrType) {
         if (elemCount == 0) {
            arrType = makeRuntimeArray(elemType);
         } else {
            auto sizeId = makeUintConstant(elemCount);
            arrType = makeArrayType(elemType, sizeId, 0);
         }

         auto baseTypeName = getTypeName(elemType);
         if (!baseTypeName.size()) {
            decaf_abort("Unexpected element type for vector type");
         }
         if (elemCount == 0) {
            addName(arrType, fmt::format("{}[]", baseTypeName).c_str());
         } else {
            addName(arrType, fmt::format("{}[{}]", baseTypeName, elemCount).c_str());
         }

         mArrType[arrPair] = arrType;
      }
      return arrType;
   }


   // ------------------------------------------------------------
   // Extension Library Access
   // ------------------------------------------------------------

   spv::Id glslStd450()
   {
      if (!mGlslStd450) {
         mGlslStd450 = import("GLSL.std.450");
         addName(mGlslStd450, "glslStd450");
      }
      return mGlslStd450;
   }


   // ------------------------------------------------------------
   // Type Names
   // ------------------------------------------------------------

   std::string getTypeName(spv::Id typeId)
   {
      for (auto &nameInst : names) {
         if (nameInst->getIdOperand(0) == typeId) {
            auto numCharOps = nameInst->getNumOperands() - 1;
            char *readName = new char[numCharOps * 4];
            for (auto i = 0; i < numCharOps; ++i) {
               auto pieceValue = nameInst->getImmediateOperand(1 + i);
               readName[i * 4 + 0] = (pieceValue >> 0) & 0xFF;
               readName[i * 4 + 1] = (pieceValue >> 8) & 0xFF;
               readName[i * 4 + 2] = (pieceValue >> 16) & 0xFF;
               readName[i * 4 + 3] = (pieceValue >> 24) & 0xFF;
            }
            std::string readNameStr(readName);
            delete[] readName;
            return readNameStr;
         }
      }
      return std::string();
   }


   // ------------------------------------------------------------
   // Byte Swapping
   // ------------------------------------------------------------

   spv::Id bswap8in16(spv::Id source)
   {
      auto sourceTypeId = getTypeId(source);

      auto inputTypeId = sourceTypeId;
      auto numComps = getNumTypeComponents(inputTypeId);
      if (isVectorType(inputTypeId)) {
         inputTypeId = getContainedTypeId(inputTypeId);
      }

      if (inputTypeId == uintType()) {
         auto xoxoConst = makeUintConstant(0xFF00FF00);
         auto oxoxConst = makeUintConstant(0x00FF00FF);
         auto shiftConst = makeUintConstant(8);

         if (numComps > 1) {
            xoxoConst = vectorizeConstant(xoxoConst, numComps);
            oxoxConst = vectorizeConstant(oxoxConst, numComps);
            shiftConst = vectorizeConstant(shiftConst, numComps);
         }

         auto xoxoBits = createBinOp(spv::Op::OpBitwiseAnd, sourceTypeId, source, xoxoConst);
         auto oxoxBits = createBinOp(spv::Op::OpBitwiseAnd, sourceTypeId, source, oxoxConst);

         auto oxoxBitsMoved = createBinOp(spv::Op::OpShiftRightLogical, sourceTypeId, xoxoBits, shiftConst);
         auto xoxoBitsMoved = createBinOp(spv::Op::OpShiftLeftLogical, sourceTypeId, oxoxBits, shiftConst);

         return createBinOp(spv::Op::OpBitwiseOr, sourceTypeId, oxoxBitsMoved, xoxoBitsMoved);
      } else if (inputTypeId == ushortType()) {
         auto xoConst = makeUint16Constant(0xFF00);
         auto oxConst = makeUint16Constant(0x00FF);
         auto shiftConst = makeUintConstant(8);

         if (numComps > 1) {
            xoConst = vectorizeConstant(xoConst, numComps);
            oxConst = vectorizeConstant(oxConst, numComps);
            shiftConst = vectorizeConstant(shiftConst, numComps);
         }

         auto xoBits = createBinOp(spv::Op::OpBitwiseAnd, sourceTypeId, source, xoConst);
         auto oxBits = createBinOp(spv::Op::OpBitwiseAnd, sourceTypeId, source, oxConst);

         auto oxBitsMoved = createBinOp(spv::Op::OpShiftRightLogical, sourceTypeId, xoBits, shiftConst);
         auto xoBitsMoved = createBinOp(spv::Op::OpShiftLeftLogical, sourceTypeId, oxBits, shiftConst);

         return createBinOp(spv::Op::OpBitwiseOr, sourceTypeId, oxBitsMoved, xoBitsMoved);
      } else {
         decaf_abort("Attempted to do 8in16 byte swap on invalid type");
      }
   }

   spv::Id bswap8in32(spv::Id source)
   {
      auto sourceTypeId = getTypeId(source);

      auto xoooConst = makeUintConstant(0xFF000000);
      auto oxooConst = makeUintConstant(0x00FF0000);
      auto ooxoConst = makeUintConstant(0x0000FF00);
      auto oooxConst = makeUintConstant(0x000000FF);
      auto littleShiftConst = makeUintConstant(8);
      auto bigShiftConst = makeUintConstant(24);

      auto inputTypeId = sourceTypeId;
      if (isVectorType(inputTypeId)) {
         auto numComps = getNumTypeComponents(inputTypeId);
         inputTypeId = getContainedTypeId(inputTypeId);

         xoooConst = vectorizeConstant(xoooConst, numComps);
         oxooConst = vectorizeConstant(oxooConst, numComps);
         ooxoConst = vectorizeConstant(ooxoConst, numComps);
         oooxConst = vectorizeConstant(oooxConst, numComps);
         littleShiftConst = vectorizeConstant(littleShiftConst, numComps);
         bigShiftConst = vectorizeConstant(bigShiftConst, numComps);
      }
      decaf_check(inputTypeId == uintType());

      auto xoooBits = createBinOp(spv::Op::OpBitwiseAnd, sourceTypeId, source, xoooConst);
      auto oxooBits = createBinOp(spv::Op::OpBitwiseAnd, sourceTypeId, source, oxooConst);
      auto ooxoBits = createBinOp(spv::Op::OpBitwiseAnd, sourceTypeId, source, ooxoConst);
      auto oooxBits = createBinOp(spv::Op::OpBitwiseAnd, sourceTypeId, source, oooxConst);

      auto xoooBitsMoved = createBinOp(spv::Op::OpShiftLeftLogical, sourceTypeId, oooxBits, bigShiftConst);
      auto oxooBitsMoved = createBinOp(spv::Op::OpShiftLeftLogical, sourceTypeId, ooxoBits, littleShiftConst);
      auto ooxoBitsMoved = createBinOp(spv::Op::OpShiftRightLogical, sourceTypeId, oxooBits, littleShiftConst);
      auto oooxBitsMoved = createBinOp(spv::Op::OpShiftRightLogical, sourceTypeId, xoooBits, bigShiftConst);

      auto xxooBitsMerged = createBinOp(spv::Op::OpBitwiseOr, sourceTypeId, xoooBitsMoved, oxooBitsMoved);
      auto xxxoBitsMerged = createBinOp(spv::Op::OpBitwiseOr, sourceTypeId, xxooBitsMerged, ooxoBitsMoved);
      return createBinOp(spv::Op::OpBitwiseOr, sourceTypeId, xxxoBitsMerged, oooxBitsMoved);
   }

   /*
   // From OpenGL ES 3.0 spec 2.1.3
   function from11uf(v) {
     const e = v >> 6;
     const m = v & 0x3F;
     if (e === 0) {
       if (m === 0) {
         return 0;
       } else {
         return Math.pow(2, -14) * (m / 64);
       }
     } else {
       if (e < 31) {
         return Math.pow(2, e - 15) * (1 + m / 64);
       } else {
         if (m === 0) {
           return 0;  // Inf
         } else {
           return 0;  // Nan
         }
       }
     }
   }
   */
   spv::Id unpackFloat11(spv::Id source)
   {
      decaf_check(getTypeId(source) == uintType());

      auto resPtr = createVariable(spv::StorageClassPrivate, floatType(), "unpackF11Res");

      auto eVal = createBinOp(spv::OpShiftRightLogical, uintType(), source, makeIntConstant(6));
      auto mVal = createBinOp(spv::OpBitwiseAnd, uintType(), source, makeUintConstant(0x3F));

      auto eIsZero = createBinOp(spv::Op::OpIEqual, boolType(), eVal, makeUintConstant(0));
      auto eIsZeroBlock = spv::Builder::If { eIsZero, spv::SelectionControlMaskNone, *this };
      {
         auto mIsZero = createBinOp(spv::Op::OpIEqual, boolType(), mVal, makeUintConstant(0));
         auto mIsZeroBlk = spv::Builder::If { mIsZero, spv::SelectionControlMaskNone, *this };
         {
            createStore(makeFloatConstant(0.0f), resPtr);
         }
         mIsZeroBlk.makeBeginElse();
         {
            auto mFlt = createUnaryOp(spv::OpConvertUToF, floatType(), mVal);
            auto mDiv64 = createBinOp(spv::OpFDiv, floatType(), mFlt, makeFloatConstant(64.0f));
            auto resVal = createBinOp(spv::OpFMul, floatType(), mDiv64, makeFloatConstant(pow(2.0f, -14)));
            createStore(resVal, resPtr);
         }
         mIsZeroBlk.makeEndIf();
      }
      eIsZeroBlock.makeBeginElse();
      {
         auto eLessThan31 = createBinOp(spv::OpULessThan, boolType(), eVal, makeUintConstant(31));
         auto eLessThan31Blk = spv::Builder::If { eLessThan31, spv::SelectionControlMaskNone, *this };
         {
            auto eMinus15 = createBinOp(spv::OpISub, uintType(), eVal, makeUintConstant(15));
            auto eMinus15Flt = createUnaryOp(spv::OpConvertUToF, floatType(), eMinus15);
            auto ePow = createBuiltinCall(floatType(), glslStd450(), GLSLstd450::GLSLstd450Pow, { makeFloatConstant(2.0f), eMinus15Flt });

            auto mFlt = createUnaryOp(spv::OpConvertUToF, floatType(), mVal);
            auto mDiv64 = createBinOp(spv::OpFDiv, floatType(), mFlt, makeFloatConstant(64.0f));
            auto mDiv64Plus1 = createBinOp(spv::OpFAdd, floatType(), mDiv64, makeFloatConstant(1.0f));
            auto resVal = createBinOp(spv::OpFMul, floatType(), ePow, mDiv64Plus1);
            createStore(resVal, resPtr);
         }
         eLessThan31Blk.makeBeginElse();
         {
            auto mIsZero = createBinOp(spv::Op::OpIEqual, boolType(), mVal, makeUintConstant(0));
            auto mIsZeroBlk = spv::Builder::If { mIsZero, spv::SelectionControlMaskNone, *this };
            {
               createStore(makeFloatConstant(0.0f), resPtr); // INF
            }
            mIsZeroBlk.makeBeginElse();
            {
               createStore(makeFloatConstant(0.0f), resPtr); // NAN
            }
            mIsZeroBlk.makeEndIf();
         }
         eLessThan31Blk.makeEndIf();
      }
      eIsZeroBlock.makeEndIf();

      return createLoad(resPtr);
   }
   /*
   // From OpenGL ES 3.0 spec 2.1.4
   function from10uf(v) {
     const e = v >> 5;
     const m = v & 0x1F;
     if (e === 0) {
       if (m === 0) {
         return 0;
       } else {
         return Math.pow(2, -14) * (m / 32);
       }
     } else {
       if (e < 31) {
         return Math.pow(2, e - 15) * (1 + m / 32);
       } else {
         if (m === 0) {
           return 0;  // Inf
         } else {
           return 0;  // Nan
         }
       }
     }
   }
   */
   spv::Id unpackFloat10(spv::Id source)
   {
      decaf_check(getTypeId(source) == uintType());

      auto resPtr = createVariable(spv::StorageClassPrivate, floatType(), "unpackF11Res");

      auto eVal = createBinOp(spv::OpShiftRightLogical, uintType(), source, makeIntConstant(5));
      auto mVal = createBinOp(spv::OpBitwiseAnd, uintType(), source, makeUintConstant(0x1F));

      auto eIsZero = createBinOp(spv::Op::OpIEqual, boolType(), eVal, makeUintConstant(0));
      auto eIsZeroBlock = spv::Builder::If { eIsZero, spv::SelectionControlMaskNone, *this };
      {
         auto mIsZero = createBinOp(spv::Op::OpIEqual, boolType(), mVal, makeUintConstant(0));
         auto mIsZeroBlk = spv::Builder::If { mIsZero, spv::SelectionControlMaskNone, *this };
         {
            createStore(makeFloatConstant(0.0f), resPtr);
         }
         mIsZeroBlk.makeBeginElse();
         {
            auto mFlt = createUnaryOp(spv::OpConvertUToF, floatType(), mVal);
            auto mDiv64 = createBinOp(spv::OpFDiv, floatType(), mFlt, makeFloatConstant(32.0f));
            auto resVal = createBinOp(spv::OpFMul, floatType(), mDiv64, makeFloatConstant(pow(2.0f, -14)));
            createStore(resVal, resPtr);
         }
         mIsZeroBlk.makeEndIf();
      }
      eIsZeroBlock.makeBeginElse();
      {
         auto eLessThan31 = createBinOp(spv::OpULessThan, boolType(), eVal, makeUintConstant(31));
         auto eLessThan31Blk = spv::Builder::If { eLessThan31, spv::SelectionControlMaskNone, *this };
         {
            auto eMinus15 = createBinOp(spv::OpISub, uintType(), eVal, makeUintConstant(15));
            auto eMinus15Flt = createUnaryOp(spv::OpConvertUToF, floatType(), eMinus15);
            auto ePow = createBuiltinCall(floatType(), glslStd450(), GLSLstd450::GLSLstd450Pow, { makeFloatConstant(2.0f), eMinus15Flt });

            auto mFlt = createUnaryOp(spv::OpConvertUToF, floatType(), mVal);
            auto mDiv64 = createBinOp(spv::OpFDiv, floatType(), mFlt, makeFloatConstant(32.0f));
            auto mDiv64Plus1 = createBinOp(spv::OpFAdd, floatType(), mDiv64, makeFloatConstant(1.0f));
            auto resVal = createBinOp(spv::OpFMul, floatType(), ePow, mDiv64Plus1);
            createStore(resVal, resPtr);
         }
         eLessThan31Blk.makeBeginElse();
         {
            auto mIsZero = createBinOp(spv::Op::OpIEqual, boolType(), mVal, makeUintConstant(0));
            auto mIsZeroBlk = spv::Builder::If { mIsZero, spv::SelectionControlMaskNone, *this };
            {
               createStore(makeFloatConstant(0.0f), resPtr); // INF
            }
            mIsZeroBlk.makeBeginElse();
            {
               createStore(makeFloatConstant(0.0f), resPtr); // NAN
            }
            mIsZeroBlk.makeEndIf();
         }
         eLessThan31Blk.makeEndIf();
      }
      eIsZeroBlock.makeEndIf();

      return createLoad(resPtr);
   }
protected:
   std::unordered_map<unsigned int, spv::Id> mUintConstants;
   std::unordered_map<float, spv::Id> mFloatConstants;

   spv::Id mSamplerType = spv::NoResult;
   spv::Id mBoolType = spv::NoResult;
   spv::Id mFloatType = spv::NoResult;
   spv::Id mUbyteType = spv::NoResult;
   spv::Id mUshortType = spv::NoResult;
   spv::Id mIntType = spv::NoResult;
   spv::Id mUintType = spv::NoResult;
   std::map<std::pair<spv::Id, int>, spv::Id> mVecType;
   std::map<std::pair<spv::Id, int>, spv::Id> mArrType;

   spv::Id mGlslStd450 = spv::NoResult;

};

} // namespace gpu

#endif // ifdef DECAF_VULKAN
