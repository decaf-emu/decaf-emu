#pragma once
#include "be2_struct.h"
#include <cstdlib>

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

template<typename Type, std::size_t Size>
class be2_array
{
public:
   using iterator = virt_ptr<Type>;
   using pointer = virt_ptr<Type>;
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

   pointer data() const
   {
      return cpu::VirtualPointer<Type> { cpu::translate(this) };
   }

   iterator begin() const
   {
      return data();
   }

   iterator end() const
   {
      return data() + Size;
   }

   iterator cbegin() const
   {
      return { data() };
   }

   iterator cend() const
   {
      return { data() + Size };
   }

private:
   value_type mValues[Size];
};

template<typename Type, std::size_t Size>
virt_ptr<Type> operator &(const be2_array<Type, Size> &ref)
{
   return ref.data();
}
