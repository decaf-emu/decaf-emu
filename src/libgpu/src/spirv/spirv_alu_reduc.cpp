#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

using namespace latte;

void Transpiler::translateAluOp2_CUBE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   // TODO: This instructure guarentees a specific ordering among its
   // two source operands accross the entire reduction.  We aren't going
   // to verify that guarentee here, but assume it holds.

   auto srcX = mSpv->readAluInstSrc(cf, group, *group.units[SQ_CHAN::Z], 0);
   auto srcY = mSpv->readAluInstSrc(cf, group, *group.units[SQ_CHAN::W], 0);
   auto srcZ = mSpv->readAluInstSrc(cf, group, *group.units[SQ_CHAN::X], 0);

   // Concise pseudocode (v2):
   //    if (|x| >= |y| && |x| >= |z|) {
   //       out = vec4(y, sign(x)*-z, 2x, x>=0 ? 0 : 1)
   //    } else if (|y| >= |x| && |y| >= |z|) {
   //       out = vec4(sign(y)*-z, x, 2y, y>=0 ? 2 : 3)
   //    } else {
   //       out = vec4(y, sign(z)*x, 2z, z>=0 ? 4 : 5)
   //    }
   // Note that CUBE reverses the order of the texture coordinates in the
   // output: out.yx = face.st

   auto zeroFConst = mSpv->makeFloatConstant(0.0f);
   auto oneFConst = mSpv->makeFloatConstant(1.0f);
   auto twoFConst = mSpv->makeFloatConstant(2.0f);
   auto threeFConst = mSpv->makeFloatConstant(3.0f);
   auto fourFConst = mSpv->makeFloatConstant(4.0f);
   auto fiveFConst = mSpv->makeFloatConstant(5.0f);

   // Normalize the inputs
   auto srcAll = mSpv->createCompositeConstruct(mSpv->float3Type(), { srcX, srcY, srcZ });
   auto normAll = mSpv->createBuiltinCall(mSpv->float3Type(), mSpv->glslStd450(), GLSLstd450::GLSLstd450Normalize, { srcAll });
   srcX = mSpv->createOp(spv::OpCompositeExtract, mSpv->floatType(), { normAll, 0 });
   srcY = mSpv->createOp(spv::OpCompositeExtract, mSpv->floatType(), { normAll, 1 });
   srcZ = mSpv->createOp(spv::OpCompositeExtract, mSpv->floatType(), { normAll, 2 });

   auto absX = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FAbs, { srcX });
   auto absY = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FAbs, { srcY });
   auto absZ = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FAbs, { srcZ });

   // if (|x| >= |y| && |x| >= |z|) {
   auto pred01_XY = mSpv->createBinOp(spv::OpFOrdGreaterThanEqual, mSpv->boolType(), absX, absY);
   auto pred01_XZ = mSpv->createBinOp(spv::OpFOrdGreaterThanEqual, mSpv->boolType(), absX, absZ);
   auto pred01 = mSpv->createBinOp(spv::OpLogicalAnd, mSpv->boolType(), pred01_XY, pred01_XZ);

   auto pred01Block = spv::Builder::If { pred01, spv::SelectionControlMaskNone, *mSpv };

   spv::Id dstX01, dstY01, dstZ01, dstW01;
   {
      // 0, 1
      auto predXgt0 = mSpv->createBinOp(spv::OpFOrdGreaterThanEqual, mSpv->boolType(), srcX, zeroFConst);
      dstW01 = mSpv->createOp(spv::OpSelect, mSpv->floatType(), { predXgt0, zeroFConst, oneFConst });

      // out = vec4(y, sign(x)*-z, 2x, x>=0 ? 0 : 1)
      auto signX = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FSign, { srcX });
      auto negZ = mSpv->createUnaryOp(spv::OpFNegate, mSpv->floatType(), srcZ);

      dstX01 = srcY;
      dstY01 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), signX, negZ);
      dstZ01 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), srcX, twoFConst);
   }
   auto dst01Block = mSpv->getBuildPoint();

   pred01Block.makeBeginElse();

   // } else if (|y| >= |x| && |y| >= |z|) {
   auto pred23_YX = mSpv->createBinOp(spv::OpFOrdGreaterThanEqual, mSpv->boolType(), absY, absX);
   auto pred23_YZ = mSpv->createBinOp(spv::OpFOrdGreaterThanEqual, mSpv->boolType(), absY, absZ);
   auto pred23 = mSpv->createBinOp(spv::OpLogicalAnd, mSpv->boolType(), pred23_YX, pred23_YZ);

   auto pred23Block = spv::Builder::If { pred23, spv::SelectionControlMaskNone, *mSpv };

   spv::Id dstX23, dstY23, dstZ23, dstW23;
   {
      // 2, 3
      auto predYgt0 = mSpv->createBinOp(spv::OpFOrdGreaterThanEqual, mSpv->boolType(), srcY, zeroFConst);
      dstW23 = mSpv->createOp(spv::OpSelect, mSpv->floatType(), { predYgt0, twoFConst, threeFConst });

      // out = vec4(sign(y)*-z, x, 2y, y>=0 ? 2 : 3)
      auto signY = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FSign, { srcY });
      auto negZ = mSpv->createUnaryOp(spv::OpFNegate, mSpv->floatType(), srcZ);

      dstX23 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), signY, negZ);
      dstY23 = srcX;
      dstZ23 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), srcY, twoFConst);
   }
   auto dst23Block = mSpv->getBuildPoint();

   pred23Block.makeBeginElse();

   // } else {
   spv::Id dstX45, dstY45, dstZ45, dstW45;
   {
      // 4, 5
      auto predZgt0 = mSpv->createBinOp(spv::OpFOrdGreaterThanEqual, mSpv->boolType(), srcZ, zeroFConst);
      dstW45 = mSpv->createOp(spv::OpSelect, mSpv->floatType(), { predZgt0, fourFConst, fiveFConst });

      // out = vec4(y, sign(z)*x, 2z, z>=0 ? 4 : 5)
      auto signZ = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FSign, { srcZ });

      dstX45 = srcY;
      dstY45 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), signZ, srcX);
      dstZ45 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), srcZ, twoFConst);
   }
   auto dst45Block = mSpv->getBuildPoint();

   pred23Block.makeEndIf();

   auto dstX2345 = mSpv->createOp(spv::OpPhi, mSpv->floatType(), { dstX23, dst23Block->getId(), dstX45, dst45Block->getId() });
   auto dstY2345 = mSpv->createOp(spv::OpPhi, mSpv->floatType(), { dstY23, dst23Block->getId(), dstY45, dst45Block->getId() });
   auto dstZ2345 = mSpv->createOp(spv::OpPhi, mSpv->floatType(), { dstZ23, dst23Block->getId(), dstZ45, dst45Block->getId() });
   auto dstW2345 = mSpv->createOp(spv::OpPhi, mSpv->floatType(), { dstW23, dst23Block->getId(), dstW45, dst45Block->getId() });

   auto dst2345Block = mSpv->getBuildPoint();

   pred01Block.makeEndIf();

   auto dstX = mSpv->createOp(spv::OpPhi, mSpv->floatType(), { dstX01, dst01Block->getId(), dstX2345, dst2345Block->getId() });
   auto dstY = mSpv->createOp(spv::OpPhi, mSpv->floatType(), { dstY01, dst01Block->getId(), dstY2345, dst2345Block->getId() });
   auto dstZ = mSpv->createOp(spv::OpPhi, mSpv->floatType(), { dstZ01, dst01Block->getId(), dstZ2345, dst2345Block->getId() });
   auto dstW = mSpv->createOp(spv::OpPhi, mSpv->floatType(), { dstW01, dst01Block->getId(), dstW2345, dst2345Block->getId() });

   // We honestly have no bloody clue why we need to do this kind of translation to the CUBE
   // data before we pass it back to the shader, but they do their own kinds of wierd translations
   // so we will just pretend like this is a normal part of shader writing and move on?...
   // It is worth mentioning that our translation is NOT the inverse of what they do, and is
   // another set of operations entirely, which kind of relates to what they do, but we move
   // the coordinates around differently...
   {
      auto absZ = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FAbs, { dstZ });

      dstX = mSpv->createBinOp(spv::OpFSub, mSpv->floatType(), dstX, absZ);
      dstY = mSpv->createBinOp(spv::OpFSub, mSpv->floatType(), dstY, absZ);
   }

   mSpv->writeAluOpDest(cf, group, SQ_CHAN::X, *group.units[SQ_CHAN::X], dstX);
   mSpv->writeAluOpDest(cf, group, SQ_CHAN::Y, *group.units[SQ_CHAN::Y], dstY);
   mSpv->writeAluOpDest(cf, group, SQ_CHAN::Z, *group.units[SQ_CHAN::Z], dstZ);
   mSpv->writeAluOpDest(cf, group, SQ_CHAN::W, *group.units[SQ_CHAN::W], dstW);
}

void Transpiler::translateAluOp2_DOT4(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   translateAluOp2_DOT4_IEEE(cf, group, unit, inst);
}

void Transpiler::translateAluOp2_DOT4_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluReducSrc(cf, group, 0);
   auto src1 = mSpv->readAluReducSrc(cf, group, 1);

   auto output = mSpv->createBinOp(spv::Op::OpDot, mSpv->floatType(), src0, src1);

   mSpv->writeAluReducDest(cf, group, output);
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
