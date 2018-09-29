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

} // namespace spirv

#endif // ifdef DECAF_VULKAN
