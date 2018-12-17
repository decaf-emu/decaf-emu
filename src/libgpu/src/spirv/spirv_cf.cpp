#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

using namespace latte;

void Transpiler::translateCf_ALU(const ControlFlowInst &cf)
{
   auto afterBlock = mSpv->startCfCondBlock();

   translateAluClause(cf);

   mSpv->endCfCondBlock(afterBlock);
}

void Transpiler::translateCf_ALU_PUSH_BEFORE(const ControlFlowInst &cf)
{
   mSpv->pushStack();
   translateCf_ALU(cf);
}

void Transpiler::translateCf_ALU_POP_AFTER(const ControlFlowInst &cf)
{
   translateCf_ALU(cf);
   mSpv->popStack(1);
}

void Transpiler::translateCf_ALU_POP2_AFTER(const ControlFlowInst &cf)
{
   translateCf_ALU(cf);
   mSpv->popStack(2);
}

void Transpiler::translateCf_ALU_EXT(const ControlFlowInst &cf)
{
   translateCf_ALU(cf);
}

void Transpiler::translateCf_ALU_CONTINUE(const ControlFlowInst &cf)
{
   translateCf_ALU(cf);

   // If state is set to Inactive, set it to InactiveContinue
   auto isInactive = mSpv->createBinOp(spv::OpIEqual, mSpv->boolType(),
                                       mSpv->createLoad(mSpv->stateVar()),
                                       mSpv->stateInactive());
   auto ifBuilder = spv::Builder::If { isInactive, spv::SelectionControlMaskNone, *mSpv };
   mSpv->createStore(mSpv->stateInactiveContinue(), mSpv->stateVar());
   ifBuilder.makeEndIf();
}

void Transpiler::translateCf_ALU_BREAK(const ControlFlowInst &cf)
{
   translateCf_ALU(cf);

   // If state is set to Inactive, set it to InactiveBreak
   auto isInactive = mSpv->createBinOp(spv::OpIEqual, mSpv->boolType(),
                                       mSpv->createLoad(mSpv->stateVar()),
                                       mSpv->stateInactive());
   auto ifBuilder = spv::Builder::If { isInactive, spv::SelectionControlMaskNone, *mSpv };
   mSpv->createStore(mSpv->stateInactiveBreak(), mSpv->stateVar());
   ifBuilder.makeEndIf();
}

void Transpiler::translateCf_ALU_ELSE_AFTER(const ControlFlowInst &cf)
{
   translateCf_ALU(cf);
   translateCf_ELSE(cf);
}

void Transpiler::translateCf_CALL_FS(const ControlFlowInst &cf)
{
   mSpv->createFunctionCall(mSpv->getFunction("fs_main"), {});
}

void Transpiler::translateCf_ELSE(const ControlFlowInst &cf)
{
   mSpv->elseStack();
}

void Transpiler::translateCf_JUMP(const ControlFlowInst &cf)
{
   // This is actually only an optimization so we can ignore it.
}

void Transpiler::translateCf_KILL(const ControlFlowInst &cf)
{
   latte::ShaderParser::translateCf_KILL(cf);
}

void Transpiler::translateCf_NOP(const ControlFlowInst &cf)
{
   // Who knows why they waste space with explicitly encoded NOP's...
}

void Transpiler::translateCf_PUSH(const ControlFlowInst &cf)
{
   mSpv->pushStack();
}

void Transpiler::translateCf_POP(const ControlFlowInst &cf)
{
   mSpv->popStack(cf.word1.POP_COUNT());
}

void Transpiler::translateCf_RETURN(const ControlFlowInst &cf)
{
   auto afterBlock = mSpv->startCfCondBlock(cf.word1.COND(), cf.word1.CF_CONST());

   mSpv->makeReturn(true);

   mSpv->endCfCondBlock(afterBlock, true);
}

void Transpiler::translateCf_TEX(const ControlFlowInst &cf)
{
   auto afterBlock = mSpv->startCfCondBlock(cf.word1.COND(), cf.word1.CF_CONST());

   translateTexClause(cf);

   mSpv->endCfCondBlock(afterBlock);
}

void Transpiler::translateCf_VTX(const ControlFlowInst &cf)
{
   auto afterBlock = mSpv->startCfCondBlock(cf.word1.COND(), cf.word1.CF_CONST());

   translateVtxClause(cf);

   mSpv->endCfCondBlock(afterBlock);
}

void Transpiler::translateCf_VTX_TC(const ControlFlowInst &cf)
{
   auto afterBlock = mSpv->startCfCondBlock(cf.word1.COND(), cf.word1.CF_CONST());

   translateVtxClause(cf);

   mSpv->endCfCondBlock(afterBlock);
}

void Transpiler::translateCf_LOOP_START(const ControlFlowInst &cf)
{
   latte::ShaderParser::translateCf_LOOP_START(cf);
}

void Transpiler::translateCf_LOOP_START_DX10(const ControlFlowInst &cf)
{
   decaf_check(cf.word1.COND() == latte::SQ_CF_COND::ACTIVE);

   mSpv->pushStack();

   auto loop = LoopState { };
   loop.startPC = mCfPC;
   loop.endPC = cf.word0.ADDR() - 1;
   loop.head = &mSpv->makeNewBlock();
   loop.body = &mSpv->makeNewBlock();
   loop.merge = &mSpv->makeNewBlock();
   loop.continue_target = &mSpv->makeNewBlock();
   mLoopStack.emplace_back(loop);

   mSpv->createBranch(loop.head);
   mSpv->setBuildPoint(loop.head);
   mSpv->createLoopMerge(loop.merge, loop.continue_target, spv::LoopControlMaskNone, 0);
   mSpv->createBranch(loop.body);
   mSpv->setBuildPoint(loop.body);

   auto stateVal = mSpv->createLoad(mSpv->stateVar());

   // If state is set to InactiveContinue, set it to Active
   {
      auto isInactiveContinue = mSpv->createBinOp(spv::OpIEqual, mSpv->boolType(),
                                                  stateVal,
                                                  mSpv->stateInactiveContinue());
      auto ifBuilder = spv::Builder::If { isInactiveContinue, spv::SelectionControlMaskNone, *mSpv };
      mSpv->createStore(mSpv->stateActive(), mSpv->stateVar());
      ifBuilder.makeEndIf();
   }

   // If state is set to Inactive, set it to InactiveBreak
   {
      auto isInactive = mSpv->createBinOp(spv::OpIEqual, mSpv->boolType(), stateVal, mSpv->stateInactive());
      auto ifBuilder = spv::Builder::If { isInactive, spv::SelectionControlMaskNone, *mSpv };
      mSpv->createStore(mSpv->stateInactiveBreak(), mSpv->stateVar());
      ifBuilder.makeEndIf();
   }
}

void Transpiler::translateCf_LOOP_START_NO_AL(const ControlFlowInst &cf)
{
   latte::ShaderParser::translateCf_LOOP_START_NO_AL(cf);
}

void Transpiler::translateCf_LOOP_END(const ControlFlowInst &cf)
{
   auto loop = mLoopStack.back();

   // Sanity check to ensure we are at the correct cfPC
   decaf_check(mCfPC == loop.endPC);
   decaf_check((cf.word0.ADDR() - 1) == loop.startPC);

   mSpv->createBranch(loop.continue_target);
   mSpv->setBuildPoint(loop.continue_target);

   // Continue while state != InactiveBreak
   auto predContinue = mSpv->createBinOp(spv::OpINotEqual, mSpv->boolType(),
                                         mSpv->createLoad(mSpv->stateVar()),
                                         mSpv->stateInactiveBreak());
   mSpv->createConditionalBranch(predContinue, loop.head, loop.merge);

   mSpv->setBuildPoint(loop.merge);
   mLoopStack.pop_back();

   // LOOP_END ignores POP_COUNT as per R600 ISA Documentation
   mSpv->popStack(1);
}

void Transpiler::translateCf_LOOP_CONTINUE(const ControlFlowInst &cf)
{
   latte::ShaderParser::translateCf_LOOP_CONTINUE(cf);
}

void Transpiler::translateCf_LOOP_BREAK(const ControlFlowInst &cf)
{
   latte::ShaderParser::translateCf_LOOP_BREAK(cf);
}

void Transpiler::translateCf_EMIT_VERTEX(const ControlFlowInst &cf)
{
   auto afterBlock = mSpv->startCfCondBlock(cf.word1.COND(), cf.word1.CF_CONST());

   mSpv->createFunctionCall(mSpv->getFunction("dc_main"), {});
   mSpv->createNoResultOp(spv::OpEmitVertex);

   mSpv->endCfCondBlock(afterBlock);
}

void Transpiler::translateCf_EMIT_CUT_VERTEX(const ControlFlowInst &cf)
{
   auto afterBlock = mSpv->startCfCondBlock(cf.word1.COND(), cf.word1.CF_CONST());

   mSpv->createFunctionCall(mSpv->getFunction("dc_main"), {});
   mSpv->createNoResultOp(spv::OpEmitVertex);
   mSpv->createNoResultOp(spv::OpEndPrimitive);

   mSpv->endCfCondBlock(afterBlock);
}

void Transpiler::translateCf_CUT_VERTEX(const ControlFlowInst &cf)
{
   auto afterBlock = mSpv->startCfCondBlock(cf.word1.COND(), cf.word1.CF_CONST());

   mSpv->createNoResultOp(spv::OpEndPrimitive);

   mSpv->endCfCondBlock(afterBlock);
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
