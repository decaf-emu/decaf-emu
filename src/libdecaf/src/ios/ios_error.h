#pragma once
#include "ios_enum.h"

namespace ios
{

constexpr IOSErrorCategory
iosGetErrorCategory(IOSError error)
{
   return static_cast<IOSErrorCategory>(((~error) >> 16) & 0x3FF);
}

constexpr int32_t
iosGetErrorCode(IOSError error)
{
   return (error & 0x8000) ? (error | 0xFFFF0000) : (error & 0xFFFF);
}

constexpr bool
iosIsKernelError(int32_t error)
{
   return error > IOSError::MaxKernelError;
}

constexpr IOSError
iosMakeError(IOSErrorCategory category, int32_t code)
{
   return static_cast<IOSError>(code >= 0 ? code : ((~category) << 16) | (code & 0xFFFF));
}

} // namespace ios
