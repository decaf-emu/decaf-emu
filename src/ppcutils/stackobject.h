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
      if (core->gpr[1] != mem::untranslate(mPtr)) {
         gLog->critical("StackObject restore did not return stack to the expected position");
      }
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