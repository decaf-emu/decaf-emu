#pragma once
#include "bigendianvalue.h"
#include "pointer.h"
#include "mmu.h"

using virt_addr = cpu::VirtualAddress;
using phys_addr = cpu::PhysicalAddress;

template<typename Type>
using virt_ptr = cpu::VirtualPointer<Type>;

template<typename Type>
using phys_ptr = cpu::PhysicalPointer<Type>;

template<typename Type>
using be2_val = cpu::BigEndianValue<Type>;

template<typename Type>
using be2_ptr = be2_val<virt_ptr<Type>>;

template<typename Type>
struct be2_struct : Type
{
};

template<typename Type>
inline virt_ptr<Type> operator &(const be2_struct<Type> &ref)
{
   return virt_ptr<Type> { cpu::translate(std::addressof(ref)) };
}

template<typename Type>
inline virt_ptr<Type> operator &(const be2_val<Type> &ref)
{
   return virt_ptr<Type> { cpu::translate(std::addressof(ref)) };
}

template<typename DstType, typename SrcType>
inline virt_ptr<DstType> virt_cast(const virt_ptr<SrcType> &src)
{
   return virt_ptr<DstType> { static_cast<virt_addr>(src) };
}

template<typename DstType, typename SrcType>
inline virt_ptr<DstType> virt_cast(const be2_ptr<SrcType> &src)
{
   return virt_ptr<DstType> { static_cast<virt_addr>(src) };
}

#include "be2_array.h"
