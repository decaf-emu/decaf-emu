#pragma once

// TODO: Replace with std::integer_sequence

template<unsigned... I>
struct index_sequence
{
   template<unsigned N>
   using append = index_sequence<I..., N>;
};

template<unsigned N>
struct make_index_sequence
{
   using type = typename make_index_sequence<N - 1>::type::template append<N - 1>;
};

template<>
struct make_index_sequence<0u>
{
   using type = index_sequence<>;
};

template<typename... Types>
using index_sequence_for = typename make_index_sequence<sizeof...(Types)>::type;
