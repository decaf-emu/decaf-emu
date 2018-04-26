#pragma once
#include <algorithm>
#include <cstdint>
#include <type_traits>
#include "be2_val.h"
#include "pointer.h"

template<typename Type>
struct be2_struct;

// Equivalent to std:true_type if type T is a virt_ptr or phys_ptr.
template<typename>
struct is_cpu_ptr : std::false_type { };

template<typename T, typename A>
struct is_cpu_ptr<cpu::Pointer<T, A>> : std::true_type { };

// Detects the actual type to be used for be2_array members.
template <typename T, typename = void>
struct be2_array_item_type;

template <typename T>
struct be2_array_item_type<T, typename std::enable_if<std::is_arithmetic<T>::value
                                                   || std::is_enum<T>::value
                                                   || is_cpu_ptr<T>::value>::type>
{
   using type = be2_val<T>;
};

template <typename T>
struct be2_array_item_type<T, typename std::enable_if<!is_cpu_ptr<T>::value
                                                   && (std::is_class<T>::value
                                                    || std::is_union<T>::value)>::type>
{
   using type = be2_struct<T>;
};


template<typename Type, uint32_t Size>
class be2_array_iterator;

template<typename Type, uint32_t Size>
class be2_array_const_iterator;

// BigEndianValue but for arrays.
template<typename Type, uint32_t Size>
class be2_array
{
public:
   using raw_value_type = Type;
   using be2_value_type = typename be2_array_item_type<Type>::type;
   using size_type = uint32_t;
   using difference_type = std::ptrdiff_t;
   using iterator = be2_array_iterator<Type, Size>;
   using const_iterator = be2_array_const_iterator<Type, Size>;

   be2_array() = default;

   template<typename U = Type>
   be2_array(std::string_view src,
             typename std::enable_if<std::is_same<char, U>::value>::type * = 0)
   {
      auto count = (src.size() < Size) ? src.size() : Size - 1;
      std::copy_n(src.begin(), count, mValues);
      mValues[src.size()] = char { 0 };
   }

   be2_array(const std::array<Type, Size> &other)
   {
      for (auto i = 0u; i < Size; ++i) {
         mValues[i] = other[i];
      }
   }

   template<typename U = Type,
            typename = typename std::enable_if<std::is_same<char, U>::value>::type>
   be2_array &operator =(const std::string_view &src)
   {
      auto count = (src.size() < Size) ? src.size() : Size - 1;
      std::copy_n(src.begin(), count, mValues);
      mValues[count] = char { 0 };
      return *this;
   }

   constexpr auto &at(size_type pos)
   {
      if (pos >= Size) {
         throw std::out_of_range("invalid be2_array<T, N> subscript");
      }

      return mValues[pos];
   }

   constexpr const auto &at(size_type pos) const
   {
      if (pos >= Size) {
         throw std::out_of_range("invalid be2_array<T, N> subscript");
      }

      return mValues[pos];
   }

   constexpr auto &operator[](size_type pos)
   {
      return mValues[pos];
   }

   constexpr const auto &operator[](size_type pos) const
   {
      return mValues[pos];
   }

   constexpr auto &front()
   {
      return mValues[0];
   }

   constexpr const auto &front() const
   {
      return mValues[0];
   }

   constexpr auto &back()
   {
      return mValues[Size - 1];
   }

   constexpr const auto &back() const
   {
      return mValues[Size - 1];
   }

   constexpr bool empty() const
   {
      return Size > 0;
   }

   constexpr size_type size() const
   {
      return Size;
   }

   constexpr size_type max_size() const
   {
      return Size;
   }

   constexpr void fill(const raw_value_type &value)
   {
      for (auto i = 0u; i < Size; ++i) {
         mValues[i] = value;
      }
   }

   constexpr auto begin()
   {
      return iterator { *this, 0 };
   }

   constexpr auto end()
   {
      return iterator { *this, Size };
   }

   constexpr auto cbegin()
   {
      return const_iterator { *this, 0 };
   }

   constexpr auto cend()
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

   constexpr auto &operator*()
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

   constexpr const auto &operator*()
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
