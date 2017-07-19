#pragma once
#include "bigendianvalue.h"
#include "pointer.h"
#include "mmu.h"
#include <cstdlib>

/*
 * Define some global short names for easy access to cpu classes
 */

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
using be2_virt_ptr = be2_ptr<Type>;

template<typename Type>
using be2_phys_ptr = be2_val<phys_ptr<Type>>;


/*
 * reinterpret_cast for virt_ptr<X> to virt_ptr<Y>
 */
template<typename DstType, typename SrcType>
inline virt_ptr<DstType> virt_cast(const virt_ptr<SrcType> &src)
{
   return virt_ptr<DstType> { static_cast<virt_addr>(src) };
}


/*
 * reinterpret_cast for be2_ptr<X> to virt_ptr<Y>
 */
template<typename DstType, typename SrcType>
inline virt_ptr<DstType> virt_cast(const be2_ptr<SrcType> &src)
{
   return virt_ptr<DstType> { static_cast<virt_addr>(src) };
}


/*
 * be2_struct is a wrapper intended to be used around struct value type members
 * in structs which reside in PPC memory. This allows operator& to be magically
 * mapped to a virt_ptr with the goal of providing more memory safety.
 */
template<typename Type>
struct be2_struct : Type
{
   // Please use virt_addrof or phys_addrof instead
   cpu::VirtualPointer<Type> operator &() = delete;
};


/*
 * Detects the actual type to be used for be2_array members.
 */
template <typename T, typename = void>
struct be2_array_item_type;

template <typename T>
struct be2_array_item_type<T, typename std::enable_if<std::is_arithmetic<T>::value
                                                   || std::is_enum<T>::value>::type>
{
   using type = be2_val<T>;
};

template <typename T>
struct be2_array_item_type<T, typename std::enable_if<std::is_class<T>::value
                                                   || std::is_union<T>::value>::type>
{
   using type = be2_struct<T>;
};


/*
 * BigEndianValue but for arrays.
 */
template<typename Type, std::size_t Size>
class be2_array
{
public:
   using virt_iterator = virt_ptr<Type>;
   using phys_iterator = phys_ptr<Type>;
   using value_type = typename be2_array_item_type<Type>::type;
   using reference = value_type &;
   using const_reference = const value_type &;

   constexpr reference operator[](std::size_t index)
   {
      return mValues[index];
   }

   constexpr const_reference operator[](std::size_t index) const
   {
      return mValues[index];
   }

   constexpr std::size_t size() const
   {
      return Size;
   }

   virt_ptr<Type> virt_data() const
   {
      return { cpu::translate(this) };
   }

   phys_ptr<Type> pys_data() const
   {
      return { cpu::translatePhysical(this) };
   }

   virt_iterator virt_begin() const
   {
      return virt_data();
   }

   virt_iterator virt_end() const
   {
      return virt_data() + Size;
   }

   virt_iterator virt_cbegin() const
   {
      return { virt_data() };
   }

   virt_iterator virt_cend() const
   {
      return { virt_data() + Size };
   }

   phys_iterator phys_begin() const
   {
      return phys_data();
   }

   phys_iterator phys_end() const
   {
      return phys_data() + Size;
   }

   phys_iterator phys_cbegin() const
   {
      return { phys_data() };
   }

   phys_iterator phys_cend() const
   {
      return { phys_data() + Size };
   }

   // Please use virt_addrof or phys_addrof instead
   cpu::VirtualPointer<Type> operator &() = delete;

private:
   value_type mValues[Size];
};


/**
 * Returns a virt_ptr to a big endian value type.
 *
 * The address of the object should be in virtual memory,
 * as in virtualMemoryBase < &ref < virtualMemoryEnd.
 */
template<typename Type>
inline virt_ptr<Type> virt_addrof(const be2_struct<Type> &ref)
{
   return virt_ptr<Type> { cpu::translate(std::addressof(ref)) };
}


template<typename Type>
inline virt_ptr<Type> virt_addrof(const be2_val<Type> &ref)
{
   return virt_ptr<Type> { cpu::translate(std::addressof(ref)) };
}


template<typename Type, std::size_t Size>
virt_ptr<Type> virt_addrof(const be2_array<Type, Size> &ref)
{
   return virt_ptr<Type> { cpu::translate(std::addressof(ref)) };
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
   return phys_ptr<Type> { cpu::translatePhysical(std::addressof(ref)) };
}


template<typename Type>
inline phys_ptr<Type> phys_addrof(const be2_val<Type> &ref)
{
   return phys_ptr<Type> { cpu::translatePhysical(std::addressof(ref)) };
}


template<typename Type, std::size_t Size>
inline phys_ptr<Type> phys_addrof(const be2_array<Type, Size> &ref)
{
   return phys_ptr<Type> { cpu::translatePhysical(std::addressof(ref)) };
}
