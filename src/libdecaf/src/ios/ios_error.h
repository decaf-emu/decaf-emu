#pragma once
#include "ios_enum.h"

namespace ios
{

constexpr ErrorCategory
getErrorCategory(Error error)
{
   return static_cast<ErrorCategory>(((~error) >> 16) & 0x3FF);
}

constexpr int32_t
getErrorCode(Error error)
{
   return (error & 0x8000) ? (error | 0xFFFF0000) : (error & 0xFFFF);
}

constexpr bool
isKernelError(int32_t error)
{
   return error > Error::MaxKernelError;
}

constexpr Error
makeError(ErrorCategory category, int32_t code)
{
   return static_cast<Error>(code >= 0 ? code : ((~category) << 16) | (code & 0xFFFF));
}

} // namespace ios
