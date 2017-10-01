#pragma once
#include "bigendianvalue.h"
#include "pointer.h"
#include "mmu.h"

#include <cstdlib>
#include <string_view>

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
 * reinterpret_cast for phys_ptr<X> to phys_ptr<Y>
 */
template<typename DstType, typename SrcType>
inline phys_ptr<DstType> phys_cast(const phys_ptr<SrcType> &src)
{
   return phys_ptr<DstType> { static_cast<phys_addr>(src) };
}


/*
 * reinterpret_cast for be2_ptr<X> to phys_ptr<Y>
 */
template<typename DstType, typename SrcType>
inline phys_ptr<DstType> phys_cast(const be2_phys_ptr<SrcType> &src)
{
   return phys_ptr<DstType> { static_cast<phys_addr>(src) };
}


/*
 * be2_struct is a wrapper intended to be used around struct value type members
 * in structs which reside in PPC memory. This is so we can use members with
 * virt_addrof or phys_addrof.
 */
template<typename Type>
struct be2_struct : public Type
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


template<typename Type, uint32_t Size>
class be2_array_iterator;

template<typename Type, uint32_t Size>
class be2_array_const_iterator;

/*
 * BigEndianValue but for arrays.
 */
template<typename Type, uint32_t Size>
class be2_array
{
public:
   using raw_value_type = Type;
   using be2_value_type = typename be2_array_item_type<Type>::type;
   using size_type = uint32_t;
   using difference_type = std::ptrdiff_t;
   using reference = be2_value_type &;
   using const_reference = const be2_value_type &;
   using iterator = be2_array_iterator<Type, Size>;
   using const_iterator = be2_array_const_iterator<Type, Size>;

   be2_array() = default;

   template<typename U = Type>
   be2_array(std::string_view src, typename std::enable_if<std::is_same<char, U>::value>::type * = 0)
   {
      decaf_check(src.size() < Size);
      std::copy(src.begin(), src.end(), mValues);
      mValues[src.size()] = char { 0 };
   }

   be2_array(const std::array<Type, Size> &other)
   {
      for (auto i = 0u; i < Size; ++i) {
         mValues[i] = other[i];
      }
   }

   template<typename U = Type, typename = typename std::enable_if<std::is_same<char, U>::value>::type>
   be2_array &operator =(const std::string_view &src)
   {
      decaf_check(src.size() < Size);
      std::copy(src.begin(), src.end(), mValues);
      mValues[src.size()] = char { 0 };
      return *this;
   }

   constexpr reference at(size_type pos)
   {
      if (pos >= Size) {
         throw std::out_of_range("invalid be2_array<T, N> subscript");
      }

      return mValues[index];
   }

   constexpr const_reference at(size_type pos) const
   {
      if (pos >= Size) {
         throw std::out_of_range("invalid be2_array<T, N> subscript");
      }

      return mValues[index];
   }

   constexpr reference operator[](std::size_t index)
   {
      return mValues[index];
   }

   constexpr const_reference operator[](std::size_t index) const
   {
      return mValues[index];
   }

   constexpr reference front()
   {
      return mValues[0];
   }

   constexpr const_reference front() const
   {
      return mValues[0];
   }

   constexpr reference back()
   {
      return mValues[Size - 1];
   }

   constexpr const_reference back() const
   {
      return mValues[Size - 1];
   }

   constexpr bool empty() const
   {
      return Size > 0;
   }

   constexpr uint32_t size() const
   {
      return Size;
   }

   constexpr uint32_t max_size() const
   {
      return Size;
   }

   constexpr void fill(const raw_value_type &value)
   {
      for (auto i = 0u; i < Size; ++i) {
         mValues[i] = value;
      }
   }

   constexpr iterator begin()
   {
      return iterator { *this, 0 };
   }

   constexpr iterator end()
   {
      return iterator { *this, Size };
   }

   constexpr const_iterator cbegin()
   {
      return const_iterator { *this, 0 };
   }

   constexpr const_iterator cend()
   {
      return const_iterator { *this, Size };
   }

   // Please use virt_addrof or phys_addrof instead
   auto operator &() = delete;

protected:
   be2_value_type mValues[Size];
};

template<typename Type, uint32_t Size>
class be2_array_iterator
{
   using array_type = be2_array<Type, Size>;
   using size_type = typename array_type::size_type;

public:
   be2_array_iterator(be2_array<Type, Size> &arr, size_type index) :
      mArray(arr),
      mIndex(index)
   {
   }

   typename array_type::reference operator*()
   {
      return mArray[mIndex];
   }

   be2_array_iterator &operator++()
   {
      mIndex++;
      return *this;
   }

   bool operator !=(be2_array_iterator &other) const
   {
      return mIndex != other.mIndex;
   }

private:
   size_type mIndex;
   array_type &mArray;
};

template<typename Type, uint32_t Size>
class be2_const_array_iterator
{
   using array_type = const be2_array<Type, Size>;
   using size_type = typename array_type::size_type;

public:
   be2_const_array_iterator(array_type &arr, size_type index) :
      mArray(arr),
      mIndex(index)
   {
   }

   typename array_type::const_reference operator*()
   {
      return mArray[mIndex];
   }

   be2_const_array_iterator &operator++()
   {
      mIndex++;
      return *this;
   }

   bool operator !=(be2_const_array_iterator &other) const
   {
      return mIndex != other.mIndex;
   }

private:
   size_type mIndex;
   array_type &mArray;
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
