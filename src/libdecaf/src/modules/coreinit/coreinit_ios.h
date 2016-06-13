#pragma once
#include "common/types.h"

namespace coreinit
{

using IOHandle = uint32_t;

static const IOHandle IOInvalidHandle = -1;

/*
Unimplemented IOS functions:
IOS_Close
IOS_CloseAsync
IOS_CloseAsyncEx
IOS_Ioctl
IOS_IoctlAsync
IOS_IoctlAsyncEx
IOS_Ioctlv
IOS_IoctlvAsync
IOS_IoctlvAsyncEx
IOS_Open
IOS_OpenAsync
IOS_OpenAsyncEx
IOS_Read
IOS_ReadAsync
IOS_ReadAsyncEx
IOS_Seek
IOS_SeekAsync
IOS_SeekAsyncEx
IOS_Write
IOS_WriteAsync
IOS_WriteAsyncEx
*/

enum class IOError
{
   OK       = 0,
   Generic  = -1
};

} // namespace coreinit
