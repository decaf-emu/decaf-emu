#ifdef DECAF_VULKAN
#include "spirv_transpiler.h"

namespace spirv
{

using namespace latte;

// TODO: We really need to make up our minds on whether these kinds of things exist
// as part of the Transpiler or the ShaderSpvBuilder...  I think that if the item
// requires access to any of the latte:: structures, it should probably exist here
// and only generic things belong in ShaderSpvBuilder.

spv::Id
Transpiler::genAluCondOp(spv::Op predOp, spv::Id lhsVal, spv::Id trueVal, spv::Id falseVal)
{
   auto trueType = mSpv->getTypeId(trueVal);
   auto falseType = mSpv->getTypeId(trueVal);
   decaf_check(trueType == falseType);

   spv::Id rhsVal;
   auto lhsValType = mSpv->getTypeId(lhsVal);
   if (mSpv->isFloatType(lhsValType)) {
      rhsVal = mSpv->makeFloatConstant(0.0f);
   } else if (mSpv->isUintType(lhsValType)) {
      rhsVal = mSpv->makeUintConstant(0);
   } else if (mSpv->isIntType(lhsValType)) {
      rhsVal = mSpv->makeUintConstant(0);
   } else {
      decaf_abort("Unexpected type passed for ALU condition");
   }

   auto pred = mSpv->createBinOp(predOp, mSpv->boolType(), lhsVal, rhsVal);
   return mSpv->createTriOp(spv::Op::OpSelect, trueType, pred, trueVal, falseVal);
}

spv::Id
Transpiler::genPredSetOp(const AluInst &inst, spv::Op predOp, spv::Id typeId, spv::Id lhsVal, spv::Id rhsVal, bool updatesPredicate)
{
   // Ensure this is an OP2, which is what we expect
   decaf_check(inst.word1.ENCODING() == SQ_ALU_ENCODING::OP2);

   auto pred = mSpv->createBinOp(predOp, mSpv->boolType(), lhsVal, rhsVal);

   if (updatesPredicate) {
      if (inst.op2.UPDATE_PRED()) {
         mSpv->createStore(pred, mSpv->predicateVar());
      }

      if (inst.op2.UPDATE_EXECUTE_MASK()) {
         auto newStateVal = mSpv->createTriOp(spv::Op::OpSelect, mSpv->intType(), pred,
                                              mSpv->stateActive(), mSpv->stateInactive());
         mSpv->createStore(newStateVal, mSpv->stateVar());
      }
   }

   spv::Id trueVal, falseVal;
   if (mSpv->isFloatType(typeId)) {
      trueVal = mSpv->makeFloatConstant(1.0f);
      falseVal = mSpv->makeFloatConstant(0.0f);
   } else if (mSpv->isUintType(typeId)) {
      trueVal = mSpv->makeUintConstant(0xffffffff);
      falseVal = mSpv->makeUintConstant(0);
   } else if (mSpv->isIntType(typeId)) {
      trueVal = mSpv->makeIntConstant(-1);
      falseVal = mSpv->makeIntConstant(0);
   } else {
      decaf_abort("Unexpected return type for predicate op");
   }

   return mSpv->createTriOp(spv::Op::OpSelect, typeId, pred, trueVal, falseVal);
}

} // namespace spirv

#endif // ifdef DECAF_VULKAN
