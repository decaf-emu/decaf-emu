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
      mAddress = phys_cast<phys_addr>(ptr);
      thread->stackPointer = ptr;

      std::uninitialized_default_construct_n(getRawPointer(), NumElements);
   }

   ~StackObject()
   {
      std::destroy_n(getRawPointer(), NumElements);

      auto thread = kernel::internal::getCurrentThread();
      auto ptr = phys_cast<uint8_t *>(thread->stackPointer);
      decaf_check(phys_cast<phys_addr>(ptr) == mAddress);
      thread->stackPointer = ptr + AlignedSize;
   }
};

template<typename Type, size_t NumElements>
class StackArray : public StackObject<Type, NumElements>
{
public:
   using StackObject::StackObject;

   constexpr uint32_t size() const
   {
      return NumElements;
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

template<std::size_t N>
inline auto
make_stack_string(const char (&hostStr)[N])
{
   StackArray<const char, N + 1> guestStr;
   std::strcpy(guestStr.getRawPointer(), hostStr);
   return guestStr;
}

} // namespace ios
