#pragma once
#include "cpu/cpu.h"
#include "mem/mem.h"

namespace ppcutils
{

template <typename Type>
class StackObject
{
public:
   StackObject()
   {
      auto core = cpu::this_core::state();
      core->gpr[1] -= AlignedSize;
      mPtr = mem::translate(core->gpr[1]);
   }

   ~StackObject()
   {
      auto core = cpu::this_core::state();
      // We should check that the stack is where we expect it to be
      //  but we can't throw from here :(
      core->gpr[1] += AlignedSize;
   }

   Type* operator->() const {
      return mPtr;
   }

   Type& operator*() const {
      return *mPtr;
   }

   operator Type*() const {
      return mPtr;
   }

private:
   const size_t AlignedSize = align_up(sizeof(Type), 4);

   Type *mPtr;

};

}