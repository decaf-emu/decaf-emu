#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

using namespace latte;

void Transpiler::translateAluOp2_CUBE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   latte::ShaderParser::translateAluOp2_CUBE(cf, group, unit, inst);
}

void Transpiler::translateAluOp2_DOT4(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   auto src0 = mSpv->readAluReducSrc(cf, group, 0);
   auto src1 = mSpv->readAluReducSrc(cf, group, 1);

   auto output = mSpv->createBinOp(spv::Op::OpDot, mSpv->floatType(), src0, src1);

   mSpv->writeAluReducDest(cf, group, output);
}

void Transpiler::translateAluOp2_DOT4_IEEE(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
{
   latte::ShaderParser::translateAluOp2_DOT4_IEEE(cf, group, unit, inst);
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
