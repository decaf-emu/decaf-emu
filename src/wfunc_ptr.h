#pragma once
#include <cstdint>
#include <ostream>
#include "p32.h"
#include "ppc.h"
#include "ppctypes.h"

ThreadState *GetCurrentFiberState();

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

   wfunc_ptr(ppcaddr_t addr) :
      address(addr)
   {
   }

   wfunc_ptr(p32<void> addr) :
      address(addr)
   {
   }

   operator ppcaddr_t() const
   {
      return address;
   }

   ReturnType operator()(Args... args);

   ppcaddr_t address;
};

template<typename ReturnType, typename... Args>
struct be_wfunc_ptr
{
   be_wfunc_ptr() :
      address(0)
   {
   }

   be_wfunc_ptr& operator=(const wfunc_ptr<ReturnType, Args...> &rhs)
   {
      address = rhs.address;
      return *this;
   }

   ReturnType operator()(Args... args)
   {
      wfunc_ptr<ReturnType, Args...> ptr(static_cast<uint32_t>(address));
      return ptr(args...);
   }

   be_val<ppcaddr_t> address;
};

#pragma pack(pop)


template<typename ReturnType, typename... Args>
static inline std::ostream &
operator<<(std::ostream &os, const wfunc_ptr<ReturnType, Args...> &val)
{
   return os << static_cast<ppcaddr_t>(val);
}

namespace ppctypes
{

template<typename ReturnType, typename... Args>
struct ppctype_converter_t<wfunc_ptr<ReturnType, Args...>>
{
   typedef wfunc_ptr<ReturnType, Args...> Type;
   static const PpcType ppc_type = PpcType::WORD;
   
   static inline void to_ppc(const Type& v, uint32_t& out)
   {
      out = v.address;
   }

   static inline Type from_ppc(uint32_t in)
   {
      return Type(in);
   }
};

} // namespace ppctypes
