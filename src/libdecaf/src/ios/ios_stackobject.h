#pragma once
#include "kernel/ios_kernel_thread.h"
#include <common/align.h>
#include <common/decaf_assert.h>
#include <libcpu/be2_struct.h>

namespace ios
{

template<typename Type>
class StackObject : public phys_ptr<Type>
{
   static constexpr size_t AlignedSize = align_up(sizeof(Type), 4);

public:
   StackObject()
   {
      auto thread = kernel::internal::getCurrentThread();
      auto ptr = phys_cast<uint8_t>(thread->stackPointer) - AlignedSize;
      mAddress = static_cast<phys_addr>(ptr);
      thread->stackPointer = ptr;
   }

   ~StackObject()
   {
      auto thread = kernel::internal::getCurrentThread();
      auto ptr = phys_cast<uint8_t>(thread->stackPointer);
      decaf_check(ptr == mAddress);
      thread->stackPointer = ptr + AlignedSize;
   }
};

} // namespace ios
