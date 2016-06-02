#pragma once
#include "cpu/cpu.h"
#include "mem/mem.h"
#include "utils/align.h"

namespace ppcutils
{

template <typename Type>
class StackObject
{
   const uint32_t AlignedSize = align_up(static_cast<uint32_t>(sizeof(Type)), 4);

public:
   StackObject()
   {
      auto core = cpu::this_core::state();
      core->gpr[1] -= AlignedSize;
      mPtr = mem::translate<Type>(core->gpr[1]);
   }

   ~StackObject()
   {
      auto core = cpu::this_core::state();
      // We should check that the stack is where we expect it to be
      //  but we can't throw from here :(
      core->gpr[1] += AlignedSize;
   }

   Type* operator->() const
   {
      return mPtr;
   }

   Type& operator*() const
   {
      return *mPtr;
   }

   operator Type*() const
   {
      return mPtr;
   }

private:
   Type *mPtr;
};

}