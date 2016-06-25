#pragma once
#include "libcpu/cpu.h"
#include "libcpu/mem.h"
#include "common/align.h"

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
      auto oldStackPtr = mem::untranslate(mPtr);
      if (core->gpr[1] != oldStackPtr) {
         gLog->critical("StackObject restore did not return stack to the expected position {:08x} != {:08x}", core->gpr[1], oldStackPtr);
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

} // namespace ppcutils
