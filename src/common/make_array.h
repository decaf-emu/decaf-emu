#pragma once
#include <array>

template <class ArrayType, class... ValueTypes>
constexpr auto make_array(ValueTypes&&... values)
   -> std::array<ArrayType, sizeof...(ValueTypes)>
{
   return { { std::forward<ValueTypes>(values)... } };
}

template<size_t Size, typename Type>
static auto make_filled_array(const Type &value)
{
   std::array<Type, Size> a;
   a.fill(value);
   return a;
}
