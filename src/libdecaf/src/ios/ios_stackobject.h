#pragma once
#include "kernel/ios_kernel_thread.h"
#include <algorithm>
#include <common/align.h>
#include <common/decaf_assert.h>
#include <libcpu/be2_struct.h>
#include <memory>

namespace ios
{

template<typename Type, size_t NumElements = 1>
class StackObject : public phys_ptr<Type>
{
   static constexpr auto
   AlignedSize = align_up(static_cast<uint32_t>(sizeof(Type) * NumElements),
                          std::max<std::size_t>(alignof(Type), 4u));

public:
   StackObject()
   {
      auto thread = kernel::internal::getCurrentThread();
      auto ptr = phys_cast<uint8_t *>(thread->stackPointer) - AlignedSize;
      phys_ptr<Type>::mAddress = phys_cast<phys_addr>(ptr);
      thread->stackPointer = ptr;

      std::uninitialized_default_construct_n(phys_ptr<Type>::get(),
                                             NumElements);
   }

   ~StackObject()
   {
      std::destroy_n(phys_ptr<Type>::get(), NumElements);

      auto thread = kernel::internal::getCurrentThread();
      auto ptr = phys_cast<uint8_t *>(thread->stackPointer);
      decaf_check(phys_cast<phys_addr>(ptr) == phys_ptr<Type>::mAddress);
      thread->stackPointer = ptr + AlignedSize;
   }
};

template<typename Type, size_t NumElements>
class StackArray : public StackObject<Type, NumElements>
{
public:
   using StackObject<Type, NumElements>::StackObject;

   constexpr uint32_t size() const
   {
      return NumElements;
   }

   constexpr auto &operator[](std::size_t index)
   {
      return phys_ptr<Type>::get()[index];
   }

   constexpr const auto &operator[](std::size_t index) const
   {
      return phys_ptr<Type>::get()[index];
   }
};

} // namespace ios
