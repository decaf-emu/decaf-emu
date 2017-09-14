#pragma once
#include "ios_enum.h"
#include <utility>

namespace ios
{

template<typename SuccessType>
class Result;

}

namespace std
{

template<typename T>
struct tuple_size<::ios::Result<T>>
   : std::integral_constant<std::size_t, 2>
{
};

template<typename T>
struct tuple_element<0, ::ios::Result<T>>
{
   using type = ::ios::Error;
};
template<typename T>
struct tuple_element<1, ::ios::Result<T>>
{
   using type = T;
};

} // namespace std

namespace ios
{

template<typename SuccessType>
class Result
{
public:
   Result(SuccessType value) :
      mError(Error::OK),
      mValue(value)
   {
   }

   Result(Error error) :
      mError(error)
   {
   }

   bool isOk() const
   {
      return mError >= Error::OK;
   }

   SuccessType value() const
   {
      return mValue;
   }

   Error error() const
   {
      return mError;
   }

public:
   Error mError;
   SuccessType mValue;
};

// TEMPLATE FUNCTION get (by index)
template<size_t _Index,
   class SuccessType>
   constexpr std::tuple_element_t<_Index, Result<SuccessType>>&
   get(Result<SuccessType>& r) _NOEXCEPT
{	// get reference to _Index element of tuple
   if constexpr (_Index == 0)
   {
      return r.mError;
   } else {
      return r.mValue;
   }
}

template<size_t _Index,
   class SuccessType>
   constexpr const std::tuple_element_t<_Index, Result<SuccessType>>&
   get(const Result<SuccessType>& r) _NOEXCEPT
{	// get const reference to _Index element of tuple
   if constexpr (_Index == 0)
   {
      return r.mError;
   } else {
      return r.mValue;
   }
}

template<size_t _Index,
   class SuccessType>
   constexpr std::tuple_element_t<_Index, Result<SuccessType>>&&
   get(Result<SuccessType>&& r) _NOEXCEPT
{	// get rvalue reference to _Index element of tuple
   typedef std::tuple_element_t<_Index, Result<SuccessType>>&& _RRtype;
   if constexpr (_Index == 0)
   {
      return std::forward<_RRtype>(r.mError);
   } else {
      return std::forward<_RRtype>(r.mValue);
   }
}

template<size_t _Index,
   class SuccessType>
   constexpr const std::tuple_element_t<_Index, Result<SuccessType>>&&
   get(const Result<SuccessType>&& r) _NOEXCEPT
{	// get const rvalue reference to _Index element of tuple
   typedef const std::tuple_element_t<_Index, Result<SuccessType>>&& _RRtype;
   if constexpr (_Index == 0)
   {
      return std::forward<_RRtype>(r.mError);
   } else {
      return std::forward<_RRtype>(r.mValue);
   }
}

/*
template<size_t _Index,
   class SuccessType>
   constexpr const tuple_element_t<_Index, Result<SuccessType>>&&
   get(const Result<SuccessType>&& _Tuple) _NOEXCEPT
{	// get const rvalue reference to _Index element of tuple
   typedef typename tuple_element<_Index, Result<SuccessType>>::_Ttype _Ttype;
   typedef const tuple_element_t<_Index, Result<SuccessType>>&& _RRtype;
   return (_STD forward<_RRtype>(((_Ttype&)_Tuple)._Myfirst._Val));
}
*/
/*
template< size_t I, class T, size_t N >
constexpr T& get( array<T,N>& a ) noexcept;
(1)	(since C++11)

template< size_t I, class T, size_t N >
constexpr T&& get( array<T,N>&& a ) noexcept;
(2)	(since C++11)

template< size_t I, class T, size_t N >
constexpr const T& get( const array<T,N>& a ) noexcept;
(3)	(since C++11)

template< size_t I, class T, size_t N >
constexpr const T&& get( const array<T,N>&& a ) noexcept;
(4
*/

} // namespace ios

