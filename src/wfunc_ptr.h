#pragma once
#include <cstdint>
#include "p32.h"

#pragma pack(push, 1)

template<typename ReturnType, typename... Args>
struct wfunc_ptr
{
   wfunc_ptr() :
      address(0)
   {
   }

   wfunc_ptr(std::nullptr_t) :
      address(0)
   {
   }

   wfunc_ptr(int addr) :
      address(addr)
   {
   }

   wfunc_ptr(addr_t addr) :
      address(addr)
   {
   }

   wfunc_ptr(p32<void> addr) :
      address(addr)
   {
   }

   operator addr_t() const
   {
      return address;
   }

   uint32_t address;
};

#pragma pack(pop)


template<typename ReturnType, typename... Args>
static inline std::ostream&
operator<<(std::ostream& os, const wfunc_ptr<ReturnType, Args...>& val)
{
   return os << static_cast<addr_t>(val);
}
