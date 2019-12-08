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

   /*
   Concise pseudocode (v3):
   if (|z| >= |x| && |z| >= |y|)
     t = -y
     s = sign(z) * x
     ma = 2z
     f = (z < 0) ? 5 : 4
   else if (|y| >= |x|)
     t = sign(y) * z
     s = x
     ma = 2y
     f = (y < 0) ? 3 : 2
   else
     t = -y
     s = sign(x) * z
     ma = 2x
     f = (x < 0) ? 1 : 0

    Note that CUBE reverses the order of the texture coordinates in the
    output: out.yx = face.st
   */


   auto zeroFConst = mSpv->makeFloatConstant(0.0f);
   auto oneFConst = mSpv->makeFloatConstant(1.0f);
   auto twoFConst = mSpv->makeFloatConstant(2.0f);
   auto threeFConst = mSpv->makeFloatConstant(3.0f);
   auto fourFConst = mSpv->makeFloatConstant(4.0f);
   auto fiveFConst = mSpv->makeFloatConstant(5.0f);

   auto absX = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FAbs, { srcX });
   auto absY = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FAbs, { srcY });
   auto absZ = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FAbs, { srcZ });

   // if (|z| >= |x| && |z| >= |y|) {
   auto pred01_ZX = mSpv->createBinOp(spv::OpFOrdGreaterThanEqual, mSpv->boolType(), absZ, absX);
   auto pred01_ZY = mSpv->createBinOp(spv::OpFOrdGreaterThanEqual, mSpv->boolType(), absZ, absY);
   auto pred01 = mSpv->createBinOp(spv::OpLogicalAnd, mSpv->boolType(), pred01_ZX, pred01_ZY);

   auto pred01Block = spv::Builder::If { pred01, spv::SelectionControlMaskNone, *mSpv };

   spv::Id dstX01, dstY01, dstZ01, dstW01;
   {
      auto signZ = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FSign, { srcZ });
      auto negY = mSpv->createUnaryOp(spv::OpFNegate, mSpv->floatType(), srcY);

      dstX01 = negY;
      dstY01 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), signZ, srcX);
      dstZ01 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), srcZ, twoFConst);

      auto predZls0 = mSpv->createBinOp(spv::OpFOrdLessThan, mSpv->boolType(), srcZ, zeroFConst);
      dstW01 = mSpv->createOp(spv::OpSelect, mSpv->floatType(), { predZls0, fiveFConst, fourFConst });
   }
   auto dst01Block = mSpv->getBuildPoint();

   pred01Block.makeBeginElse();

   // } else if (|y| >= |x|) {
   auto pred23 = mSpv->createBinOp(spv::OpFOrdGreaterThanEqual, mSpv->boolType(), absY, absX);

   auto pred23Block = spv::Builder::If { pred23, spv::SelectionControlMaskNone, *mSpv };

   spv::Id dstX23, dstY23, dstZ23, dstW23;
   {
      auto signY = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FSign, { srcY });

      dstX23 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), signY, srcZ);
      dstY23 = srcX;
      dstZ23 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), srcY, twoFConst);

      auto predYls0 = mSpv->createBinOp(spv::OpFOrdLessThan, mSpv->boolType(), srcY, zeroFConst);
      dstW23 = mSpv->createOp(spv::OpSelect, mSpv->floatType(), { predYls0, threeFConst, twoFConst });
   }
   auto dst23Block = mSpv->getBuildPoint();

   pred23Block.makeBeginElse();

   // } else {
   spv::Id dstX45, dstY45, dstZ45, dstW45;
   {
      auto signX = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FSign, { srcX });
      auto negY = mSpv->createUnaryOp(spv::OpFNegate, mSpv->floatType(), srcY);

      dstX45 = negY;
      dstY45 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), signX, srcZ);
      dstZ45 = mSpv->createBinOp(spv::OpFMul, mSpv->floatType(), srcX, twoFConst);

      auto predXls0 = mSpv->createBinOp(spv::OpFOrdLessThan, mSpv->boolType(), srcX, zeroFConst);
      dstW45 = mSpv->createOp(spv::OpSelect, mSpv->floatType(), { predXls0, oneFConst, zeroFConst });
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

   // Okay, we now have a sorta bloody clue why we need to do this.  The CUBE instruction
   // intentionally performs a some math to the values such that the values shift out of
   // the 0.0-1.0 range and into the 1.0-2.0 range which has the benefit of a constant
   // mantissa component to the floating point number.  Supposedly this improves the
   // performance of the lookup for AMD hardware.  We would normally duplicate this
   // behaviour, but need to undo it because the samplers are set to BORDER, which
   // causes invalid values to be sampled.  We're not sure how its meant to work with a
   // non-wrapping sampler, but thats a problem for another day.
   {
      auto dstAbsZ = mSpv->createBuiltinCall(mSpv->floatType(), mSpv->glslStd450(), GLSLstd450::GLSLstd450FAbs, { dstZ });

      dstX = mSpv->createBinOp(spv::OpFSub, mSpv->floatType(), dstX, dstAbsZ);
      dstY = mSpv->createBinOp(spv::OpFSub, mSpv->floatType(), dstY, dstAbsZ);
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
