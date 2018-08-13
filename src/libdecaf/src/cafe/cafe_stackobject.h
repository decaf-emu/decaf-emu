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
      auto oldStackTop = virt_addr { core->gpr[1] };
      auto newStackTop = oldStackTop - AlignedSize;
      auto ptrAddr = newStackTop + 8;

      memmove(virt_cast<void *>(newStackTop).getRawPointer(),
              virt_cast<void *>(oldStackTop).getRawPointer(),
              8);

      core->gpr[1] = newStackTop.getAddress();
      mAddress = virt_addr { newStackTop + 8 };
      std::uninitialized_default_construct_n(getRawPointer(),
                                             NumElements);
   }

   ~StackObject()
   {
      std::destroy_n(getRawPointer(), NumElements);

      auto core = cpu::this_core::state();
      auto oldStackTop = virt_addr { core->gpr[1] };
      auto newStackTop = virt_addr { core->gpr[1] + AlignedSize };
      auto ptrAddr = oldStackTop + 8;
      decaf_check(mAddress == ptrAddr);

      core->gpr[1] = newStackTop.getAddress();
      memmove(virt_cast<void *>(newStackTop).getRawPointer(),
              virt_cast<void *>(oldStackTop).getRawPointer(),
              8);
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
      return getRawPointer()[index];
   }

   constexpr const auto &operator[](std::size_t index) const
   {
      return getRawPointer()[index];
   }
};

class StackString : public virt_ptr<char>
{
public:
   StackString(std::string_view hostString) :
      mSize(align_up(static_cast<uint32_t>(hostString.size()) + 1, 4))
   {
      auto core = cpu::this_core::state();
      auto oldStackTop = virt_addr { core->gpr[1] };
      auto newStackTop = oldStackTop - mSize;
      auto ptrAddr = newStackTop + 8;

      memmove(virt_cast<void *>(newStackTop).getRawPointer(),
              virt_cast<void *>(oldStackTop).getRawPointer(),
              8);

      core->gpr[1] = newStackTop.getAddress();
      mAddress = virt_addr { newStackTop + 8 };

      std::memcpy(getRawPointer(), hostString.data(), hostString.size());
      getRawPointer()[hostString.size()] = char { 0 };
   }

   StackString(StackString &&from) :
      mSize(from.mSize)
   {
      from.mSize = 0u;
      mAddress = from.mAddress;
   }

   ~StackString()
   {
      if (mSize) {
         auto core = cpu::this_core::state();
         auto oldStackTop = virt_addr { core->gpr[1] };
         auto newStackTop = virt_addr { core->gpr[1] + mSize };
         auto ptrAddr = oldStackTop + 8;
         decaf_check(mAddress == ptrAddr);

         core->gpr[1] = newStackTop.getAddress();
         memmove(virt_cast<void *>(newStackTop).getRawPointer(),
                 virt_cast<void *>(oldStackTop).getRawPointer(),
                 8);
      }
   }

   StackString &operator =(StackString &&from)
   {
      mSize = from.mSize;
      mAddress = from.mAddress;
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
