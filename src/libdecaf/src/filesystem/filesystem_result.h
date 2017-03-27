#pragma once
#include "filesystem_error.h"

namespace fs
{

template<typename ValueType>
class Result
{
public:
   Result(ValueType &&value) :
      mError(Error::OK),
      mValue(std::move(value))
   {
   }

   Result(const ValueType &value) :
      mError(Error::OK),
      mValue(value)
   {
   }

   Result(Error error) :
      mError(error)
   {
   }

   Result(Error error, const ValueType &value) :
      mError(error),
      mValue(value)
   {
   }

   ValueType value()
   {
      return mValue;
   }

   Error error() const
   {
      return mError;
   }

   explicit operator bool() const
   {
      return mError == Error::OK;
   }

   operator Error() const
   {
      return mError;
   }

   operator ValueType() const
   {
      return mValue;
   }

private:
   Error mError;
   ValueType mValue;
};

template<>
class Result<Error>
{
public:
   Result(Error error) :
      mError(error)
   {
   }

   Error error() const
   {
      return mError;
   }

   explicit operator bool() const
   {
      return mError == Error::OK;
   }

   operator Error() const
   {
      return mError;
   }

private:
   Error mError;
};

} // namespace fs
