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

template<typename Type, size_t Size>
class StackArray : public phys_ptr<Type>
{
   static constexpr size_t AlignedSize = align_up(sizeof(Type) * Size, 4);

public:
   StackArray()
   {
      auto thread = kernel::internal::getCurrentThread();
      auto ptr = phys_cast<uint8_t>(thread->stackPointer) - AlignedSize;
      mAddress = static_cast<phys_addr>(ptr);
      thread->stackPointer = ptr;
   }

   ~StackArray()
   {
      auto thread = kernel::internal::getCurrentThread();
      auto ptr = phys_cast<uint8_t>(thread->stackPointer);
      decaf_check(ptr == mAddress);
      thread->stackPointer = ptr + AlignedSize;
   }

   constexpr uint32_t size() const
   {
      return Size;
   }

   constexpr auto &operator[](std::size_t index)
   {
      return getRawPointer()[index];
   }

   constexpr const auto &operator[](std::size_t index) const
   {
      return getRawPointer()[index];
   }
};

} // namespace ios
