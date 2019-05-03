#pragma once
#include <algorithm>
#include <common/align.h>
#include <common/decaf_assert.h>
#include <libcpu/cpu.h>
#include <libcpu/be2_struct.h>
#include <memory>
#include <string_view>

namespace cafe
{

// #define DECAF_CHECK_STACK_CAFE_OBJECTS

template<typename Type, size_t NumElements = 1>
class StackObject : public virt_ptr<Type>
{
   static constexpr auto
   AlignedSize = align_up(static_cast<uint32_t>(sizeof(Type) * NumElements),
                          std::max<std::size_t>(alignof(Type), 4u));

public:
   StackObject()
   {
      auto core = cpu::this_core::state();

      // Adjust stack
      auto oldStackTop = virt_addr { core->gpr[1] };
      auto newStackTop = oldStackTop - AlignedSize;
      core->gpr[1] = static_cast<uint32_t>(newStackTop);

      // Initialise object
      virt_ptr<Type>::mAddress = newStackTop;
      std::uninitialized_default_construct_n(virt_ptr<Type>::get(),
                                             NumElements);
   }

   ~StackObject()
   {
      auto core = cpu::this_core::state();

      // Destroy object
      std::destroy_n(virt_ptr<Type>::get(), NumElements);

      // Adjust stack
      auto oldStackTop = virt_addr { core->gpr[1] };
      auto newStackTop = oldStackTop + AlignedSize;
      core->gpr[1] = newStackTop.getAddress();

#ifdef DECAF_CHECK_STACK_CAFE_OBJECTS
      decaf_check(virt_ptr<Type>::mAddress == oldStackTop);
#endif
   }

   // Disable copy
   StackObject(const StackObject &) = delete;
   StackObject &operator=(const StackObject&) = delete;

   // Disable move
   StackObject(StackObject &&) noexcept = delete;
   StackObject &operator=(StackObject&&) noexcept = delete;
};

template<typename Type, size_t NumElements>
class StackArray : public StackObject<Type, NumElements>
{
public:
   using StackObject<Type, NumElements>::StackObject;

   // Disable copy
   StackArray(const StackArray &) = delete;
   StackArray &operator=(const StackArray &) = delete;

   // Disable move
   StackArray(StackArray &&) noexcept = delete;
   StackArray &operator=(StackArray &&) noexcept = delete;

   constexpr uint32_t size() const
   {
      return NumElements;
   }

   constexpr auto &operator[](std::size_t index)
   {
      return virt_ptr<Type>::get()[index];
   }

   constexpr const auto &operator[](std::size_t index) const
   {
      return virt_ptr<Type>::get()[index];
   }
};

class StackString : public virt_ptr<char>
{
public:
   StackString(std::string_view hostString) :
      mSize(align_up(static_cast<uint32_t>(hostString.size()) + 1, 4))
   {
      auto core = cpu::this_core::state();

      // Adjust stack
      auto oldStackTop = virt_addr { core->gpr[1] };
      auto newStackTop = oldStackTop - mSize;
      core->gpr[1] = newStackTop.getAddress();

      // Initialise string
      virt_ptr<char>::mAddress = virt_addr { newStackTop };
      std::memcpy(virt_ptr<char>::get(), hostString.data(), hostString.size());
      virt_ptr<char>::get()[hostString.size()] = char { 0 };
   }

   ~StackString()
   {
      if (mSize) {
         auto core = cpu::this_core::state();

         // Adjust stack
         auto oldStackTop = virt_addr { core->gpr[1] };
         auto newStackTop = oldStackTop + mSize;
         core->gpr[1] = newStackTop.getAddress();

#ifdef DECAF_CHECK_STACK_CAFE_OBJECTS
         decaf_check(virt_ptr<char>::mAddress == oldStackTop);
#endif
      }
   }

   // Disable copy
   StackString(const StackString &) = delete;
   StackString &operator=(const StackString &) = delete;

   StackString(StackString &&from) :
      mSize(from.mSize)
   {
      from.mSize = 0u;
      virt_ptr<char>::mAddress = from.mAddress;
   }

   StackString &operator =(StackString &&from)
   {
      mSize = from.mSize;
      virt_ptr<char>::mAddress = from.mAddress;
      from.mSize = 0u;
      return *this;
   }

private:
   uint32_t mSize;
};

inline StackString
make_stack_string(std::string_view str)
{
   return { str };
}

} // namespace cafe
