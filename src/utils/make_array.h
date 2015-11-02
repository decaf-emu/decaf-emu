#pragma once
#include <array>

template <class ArrayType, class... ValueTypes>
constexpr auto make_array(ValueTypes&&... values)
   -> std::array<ArrayType, sizeof...(ValueTypes)>
{
   return { { std::forward<ValueTypes>(values)... } };
}
