#pragma once
#include <ostream>
#include "common/bitutils.h"
#include "ppctypes.h"
#include "common/virtual_ptr.h"

#pragma pack(push, 1)

template<typename ReturnType, typename... Args>
struct be_wfunc_ptr;

template<typename ReturnType, typename... Args>
struct wfunc_ptr
{
   wfunc_ptr()
   {
      setAddress(0);
   }

   wfunc_ptr(std::nullptr_t)
   {
      setAddress(0);
   }

   wfunc_ptr(ppcaddr_t addr)
   {
      setAddress(addr);
   }

   wfunc_ptr(be_wfunc_ptr<ReturnType, Args...> func)
   {
      setAddress(func.getAddress());
   }

   template<bool Endian>
   wfunc_ptr(const virtual_ptr<void, Endian> &ptr)
   {
      setAddress(ptr.getAddress());
   }

   void setAddress(ppcaddr_t addr)
   {
      address = addr;
   }

   ppcaddr_t getAddress() const
   {
      return address;
   }

   operator ppcaddr_t() const
   {
      return getAddress();
   }

   ReturnType operator()(Args... args);

   ppcaddr_t address;
};

template<typename ReturnType, typename... Args>
struct be_wfunc_ptr
{
   be_wfunc_ptr()
   {
      setAddress(0);
   }

   be_wfunc_ptr(wfunc_ptr<ReturnType, Args...> func)
   {
      setAddress(func.getAddress());
   }

   void setAddress(ppcaddr_t addr)
   {
      address = byte_swap(addr);
   }

   ppcaddr_t getAddress() const
   {
      return byte_swap(address);
   }

   be_wfunc_ptr& operator=(const wfunc_ptr<ReturnType, Args...> &rhs)
   {
      setAddress(rhs.address);
      return *this;
   }

   ReturnType operator()(Args... args)
   {
      wfunc_ptr<ReturnType, Args...> ptr(getAddress());
      return ptr(args...);
   }

   operator ppcaddr_t() const
   {
      return getAddress();
   }

   ppcaddr_t address;
};

#pragma pack(pop)

template<typename ReturnType, typename... Args>
inline std::ostream &
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
