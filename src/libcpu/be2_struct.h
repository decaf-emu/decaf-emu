#pragma once
#include "address.h"
#include "be2_array.h"
#include "be2_val.h"
#include "functionpointer.h"
#include "pointer.h"
#include "mmu.h"

#include <cstdint>
#include <cstdlib>
#include <common/cbool.h>
#include <common/structsize.h>
#include <string_view>

using virt_addr = cpu::VirtualAddress;
using phys_addr = cpu::PhysicalAddress;

using virt_addr_range = cpu::VirtualAddressRange;
using phys_addr_range = cpu::PhysicalAddressRange;

template<typename T>
using virt_ptr = cpu::Pointer<T, virt_addr>;

template<typename T>
using phys_ptr = cpu::Pointer<T, phys_addr>;

template<typename Ft>
using virt_func_ptr = cpu::FunctionPointer<virt_addr, Ft>;

template<typename Ft>
using phys_func_ptr = cpu::FunctionPointer<phys_addr, Ft>;

template<typename Type>
using be2_ptr = be2_val<virt_ptr<Type>>;

template<typename Type>
using be2_virt_ptr = be2_ptr<Type>;

template<typename Type>
using be2_phys_ptr = be2_val<phys_ptr<Type>>;

template<typename Ft>
using be2_virt_func_ptr = be2_val<virt_func_ptr<Ft>>;

template<typename Ft>
using be2_phys_func_ptr = be2_val<phys_func_ptr<Ft>>;

/*
* be2_struct is a wrapper intended to be used around struct value type members
* in structs which reside in PPC memory. This is so we can use members with
* virt_addrof or phys_addrof.
*/
template<typename Type>
struct be2_struct : public Type
{
   using Type::Type;
   using Type::operator =;

   // Please use virt_addrof or phys_addrof instead
   auto operator &() = delete;
};

// reinterpret_cast for virt_addr to virt_ptr<X>
template<typename DstType, typename = std::enable_if<std::is_pointer<DstType>::value>::type>
inline auto virt_cast(virt_addr src)
{
   return cpu::pointer_cast_impl<cpu::VirtualAddress, virt_addr, DstType>::cast(src);
}

// reinterpret_cast for virt_ptr<X> to virt_ptr<Y> or virt_addr
template<typename DstType, typename SrcType, typename = std::enable_if<std::is_pointer<DstType>::value || std::is_same<DstType, virt_addr>::value>::type>
inline auto virt_cast(const virt_ptr<SrcType> &src)
{
   return cpu::pointer_cast_impl<cpu::VirtualAddress, SrcType, DstType>::cast(src);
}

// reinterpret_cast for be2_ptr<X> to virt_ptr<Y> or virt_addr
template<typename DstType, typename SrcType, typename = std::enable_if<std::is_pointer<DstType>::value || std::is_same<DstType, virt_addr>::value>::type>
inline auto virt_cast(const be2_virt_ptr<SrcType> &src)
{
   return cpu::pointer_cast_impl<cpu::VirtualAddress, SrcType, DstType>::cast(src);
}

// reinterpret_cast for phys_addr to phys_ptr<X>
template<typename DstType, typename = std::enable_if<std::is_pointer<DstType>::value || std::is_same<DstType, virt_addr>::value>::type>
inline auto phys_cast(phys_addr src)
{
   return cpu::pointer_cast_impl<cpu::PhysicalAddress, phys_addr, DstType>::cast(src);
}

// reinterpret_cast for phys_ptr<X> to phys_ptr<Y> or phys_addr
template<typename DstType, typename SrcType, typename = std::enable_if<std::is_pointer<DstType>::value || std::is_same<DstType, phys_addr>::value>::type>
inline auto phys_cast(const phys_ptr<SrcType> &src)
{
   return cpu::pointer_cast_impl<cpu::PhysicalAddress, SrcType, DstType>::cast(src);
}

// reinterpret_cast for be2_ptr<X> to phys_ptr<Y> or phys_addr
template<typename DstType, typename SrcType, typename = std::enable_if<std::is_pointer<DstType>::value || std::is_same<DstType, phys_addr>::value>::type>
inline auto phys_cast(const be2_phys_ptr<SrcType> &src)
{
   return cpu::pointer_cast_impl<cpu::PhysicalAddress, SrcType, DstType>::cast(src);
}

// reinterpret_cast for virt_addr to virt_func_ptr<X>
template<typename FunctionType>
inline auto virt_func_cast(virt_addr src)
{
   return cpu::func_pointer_cast_impl<cpu::VirtualAddress, FunctionType>::cast(src);
}

// reinterpret_cast for virt_func_ptr<X> to virt_addr
template<typename FunctionType>
inline auto virt_func_cast(virt_func_ptr<FunctionType> src)
{
   return cpu::func_pointer_cast_impl<cpu::VirtualAddress, FunctionType>::cast(src);
}

// reinterpret_cast for phys_addr to phys_func_ptr<X>
template<typename FunctionType>
inline auto phys_func_cast(phys_addr src)
{
   return cpu::func_pointer_cast_impl<cpu::PhysicalAddress, FunctionType>::cast(src);
}

// reinterpret_cast for phys_func_ptr<X> to phys_addr
template<typename FunctionType>
inline auto phys_func_cast(phys_func_ptr<FunctionType> src)
{
   return cpu::func_pointer_cast_impl<cpu::PhysicalAddress, FunctionType>::cast(src);
}

/**
 * Returns a virt_ptr to a big endian value type.
 *
 * The address of the object should be in virtual memory,
 * as in virtualMemoryBase < &ref < virtualMemoryEnd.
 */
template<typename Type>
inline virt_ptr<Type> virt_addrof(const be2_struct<Type> &ref)
{
   return virt_cast<Type *>(cpu::translate(std::addressof(ref)));
}

template<typename Type>
inline virt_ptr<Type> virt_addrof(const be2_val<Type> &ref)
{
   return virt_cast<Type *>(cpu::translate(std::addressof(ref)));
}

template<typename Type, std::size_t Size>
virt_ptr<Type> virt_addrof(const be2_array<Type, Size> &ref)
{
   return virt_cast<Type *>(cpu::translate(std::addressof(ref)));
}


/**
 * Returns a phys_ptr to a big endian value type.
 *
 * The address of the object should be in physical memory,
 * as in physicalMemoryBase < &ref < physicalMemoryEnd.
 *
 * Will not auto translate a virtual address to a physical address, for that
 * you should use cpu::virtualToPhysicalAddress(virt_addrof(x)).
 */
template<typename Type>
inline phys_ptr<Type> phys_addrof(const be2_struct<Type> &ref)
{
   return phys_cast<Type *>(cpu::translatePhysical(std::addressof(ref)));
}

template<typename Type>
inline phys_ptr<Type> phys_addrof(const be2_val<Type> &ref)
{
   return phys_cast<Type *>(cpu::translatePhysical(std::addressof(ref)));
}

template<typename Type, std::size_t Size>
inline phys_ptr<Type> phys_addrof(const be2_array<Type, Size> &ref)
{
   return phys_cast<Type *>(cpu::translatePhysical(std::addressof(ref)));
}

// Equivalent to std:true_type if type T is a virt_ptr.
template<typename>
struct is_virt_ptr : std::false_type { };

template<typename T>
struct is_virt_ptr<virt_ptr<T>> : std::true_type { };

// Equivalent to std:true_type if type T is a phys_ptr.
template<typename>
struct is_phys_ptr : std::false_type { };

template<typename T>
struct is_phys_ptr<virt_ptr<T>> : std::true_type { };
