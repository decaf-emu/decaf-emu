#pragma once
#include "coreinit_enum.h"
#include "ppcutils/wfunc_ptr.h"
#include "ios/ios_ipc.h"

#include <cstdint>

namespace coreinit
{

/**
 * \defgroup coreinit_ios IOS
 * \ingroup coreinit
 * @{
 */

/*
Unimplemented IOS functions:
IOS_OpenAsyncEx
IOS_CloseAsyncEx
IOS_IoctlAsyncEx
IOS_IoctlvAsyncEx

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

using IOSCommand = ios::Command;
using IOSError = ios::Error;
using IOSErrorCategory = ios::ErrorCategory;
using IOSHandle = int32_t;
using IOSOpenMode = ios::OpenMode;
using IOSVec = ios::IoctlVec;
using IOSAsyncCallbackFn = wfunc_ptr<void, IOSError /* status */, void * /* context */>;

IOSError
IOS_Open(const char *device,
         IOSOpenMode mode);

IOSError
IOS_OpenAsync(const char *device,
              IOSOpenMode mode,
              IOSAsyncCallbackFn callback,
              void *context);

IOSError
IOS_Close(IOSHandle handle);

IOSError
IOS_CloseAsync(IOSHandle handle,
               IOSAsyncCallbackFn callback,
               void *context);

IOSError
IOS_Ioctl(IOSHandle handle,
          uint32_t request,
          void *inBuf,
          uint32_t inLen,
          void *outBuf,
          uint32_t outLen);

IOSError
IOS_IoctlAsync(IOSHandle handle,
               uint32_t request,
               void *inBuf,
               uint32_t inLen,
               void *outBuf,
               uint32_t outLen,
               IOSAsyncCallbackFn callback,
               void *context);

IOSError
IOS_Ioctlv(IOSHandle handle,
           uint32_t request,
           uint32_t vecIn,
           uint32_t vecOut,
           IOSVec *vec);

IOSError
IOS_IoctlvAsync(IOSHandle handle,
                uint32_t request,
                uint32_t vecIn,
                uint32_t vecOut,
                IOSVec *vec,
                IOSAsyncCallbackFn callback,
                void *context);

/** @} */

} // namespace coreinit
