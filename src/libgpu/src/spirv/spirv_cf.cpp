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

   // state = (state == Active) ? Active : InactiveContinue
   auto stateVar = mSpv->stateVar();
   auto state = mSpv->createLoad(stateVar);
   auto pred = mSpv->createBinOp(spv::OpIEqual, mSpv->boolType(), state, mSpv->stateActive());
   auto newState = mSpv->createTriOp(spv::OpSelect, mSpv->intType(),
                                     pred,
                                     mSpv->stateActive(),
                                     mSpv->stateInactiveContinue());

   mSpv->createStore(newState, stateVar);
}

void Transpiler::translateCf_ALU_BREAK(const ControlFlowInst &cf)
{
   translateCf_ALU(cf);

   // state = (state == Active) ? Active : InactiveContinue
   auto stateVar = mSpv->stateVar();
   auto state = mSpv->createLoad(stateVar);
   auto pred = mSpv->createBinOp(spv::OpIEqual, mSpv->boolType(), state, mSpv->stateActive());
   auto newState = mSpv->createTriOp(spv::OpSelect, mSpv->intType(),
                                     pred,
                                     mSpv->stateActive(),
                                     mSpv->stateInactiveBreak());

   mSpv->createStore(newState, stateVar);
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
   auto stateVar = mSpv->stateVar();
   auto state = mSpv->createLoad(stateVar);
   auto pred = mSpv->createBinOp(spv::OpIEqual, mSpv->boolType(), state, mSpv->stateActive());

   auto newState = mSpv->createTriOp(spv::OpSelect, mSpv->intType(),
                                     pred,
                                     mSpv->stateInactive(),
                                     mSpv->stateActive());

   mSpv->createStore(newState, stateVar);
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

   auto headerBlock = &mSpv->makeNewBlock();
   auto bodyBlock = &mSpv->makeNewBlock();
   auto continueBlock = &mSpv->makeNewBlock();
   auto afterBlock = &mSpv->makeNewBlock();

   mSpv->createBranch(headerBlock);
   mSpv->setBuildPoint(headerBlock);

   mSpv->createLoopMerge(afterBlock, continueBlock, 0, 0);
   mSpv->createBranch(bodyBlock);
   mSpv->setBuildPoint(bodyBlock);

   LoopState loop;
   loop.startPC = mCfPC;
   loop.endPC = cf.word0.ADDR() - 1;
   loop.headerBlock = headerBlock;
   loop.bodyBlock = bodyBlock;
   loop.continueBlock = continueBlock;
   loop.afterBlock = afterBlock;
   loopStack.push_back(loop);
}

void Transpiler::translateCf_LOOP_START_NO_AL(const ControlFlowInst &cf)
{
   latte::ShaderParser::translateCf_LOOP_START_NO_AL(cf);
}

void Transpiler::translateCf_LOOP_END(const ControlFlowInst &cf)
{
   // TODO: LOOP_END has different behaviour depending on which LOOP_START
   // instruction started the loop, currently we only handle LOOP_START_DX10
   auto &loopState = loopStack.back();
   auto loopIndex = loopStack.size() - 1;
   loopStack.pop_back();

   // Sanity check to ensure we are at the cfPC
   decaf_check(mCfPC == loopState.endPC);
   decaf_check((cf.word0.ADDR() - 1) == loopState.startPC);

   auto stateVal = mSpv->createLoad(mSpv->stateVar());

   auto predBreak = mSpv->createBinOp(spv::OpIEqual, mSpv->boolType(), stateVal, mSpv->stateInactiveBreak());
   auto noBreakBlock = &mSpv->makeNewBlock();
   mSpv->createConditionalBranch(predBreak, loopState.afterBlock, noBreakBlock);
   mSpv->setBuildPoint(noBreakBlock);

   auto predContinue = mSpv->createBinOp(spv::OpIEqual, mSpv->boolType(), stateVal, mSpv->stateInactiveContinue());
   auto continueBlock = spv::Builder::If { predContinue, spv::SelectionControlMaskNone, *mSpv };
   mSpv->createStore(mSpv->stateActive(), mSpv->stateVar());
   continueBlock.makeEndIf();

   mSpv->createBranch(loopState.continueBlock);
   mSpv->setBuildPoint(loopState.continueBlock);

   // We don't do anything

   mSpv->createBranch(loopState.headerBlock);
   mSpv->setBuildPoint(loopState.afterBlock);
}

void Transpiler::translateCf_LOOP_CONTINUE(const ControlFlowInst &cf)
{
   latte::ShaderParser::translateCf_LOOP_CONTINUE(cf);
}

void Transpiler::translateCf_LOOP_BREAK(const ControlFlowInst &cf)
{
   latte::ShaderParser::translateCf_LOOP_BREAK(cf);
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
