#pragma once
#include "vfs_error.h"
#include <utility>

namespace vfs
{

template<typename ValueType>
class Result
{
public:
   Result(ValueType value) :
      mError(Error::Success),
      mValue(std::move(value))
   {
   }

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
      return mError == Error::Success;
   }

   ValueType &operator *()
   {
      return mValue;
   }

   const ValueType &operator *() const
   {
      return mValue;
   }

   ValueType *operator ->()
   {
      return &mValue;
   }

   const ValueType *operator ->() const
   {
      return &mValue;
   }

private:
   Error mError;
   ValueType mValue;
};

} // namespace vfs
