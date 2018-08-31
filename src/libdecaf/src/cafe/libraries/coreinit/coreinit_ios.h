#pragma once
#include "coreinit_enum.h"
#include "ios/ios_ipc.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
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

#define IOS_FAILED(error) (error < ios::Error::OK)
#define IOS_SUCCESS(error) (error >= ios::Error::OK)

using IOSCommand = ios::Command;
using IOSError = ios::Error;
using IOSErrorCategory = ios::ErrorCategory;
using IOSHandle = int32_t;
using IOSOpenMode = ios::OpenMode;
using IOSVec = ios::IoctlVec;
using IOSAsyncCallbackFn = virt_func_ptr<void(IOSError status,
                                              virt_ptr<void> context)>;

IOSError
IOS_Open(virt_ptr<const char> device,
         IOSOpenMode mode);

IOSError
IOS_OpenAsync(virt_ptr<const char> device,
              IOSOpenMode mode,
              IOSAsyncCallbackFn callback,
              virt_ptr<void> context);

IOSError
IOS_Close(IOSHandle handle);

IOSError
IOS_CloseAsync(IOSHandle handle,
               IOSAsyncCallbackFn callback,
               virt_ptr<void> context);

IOSError
IOS_Ioctl(IOSHandle handle,
          uint32_t request,
          virt_ptr<void> inBuf,
          uint32_t inLen,
          virt_ptr<void> outBuf,
          uint32_t outLen);

IOSError
IOS_IoctlAsync(IOSHandle handle,
               uint32_t request,
               virt_ptr<void> inBuf,
               uint32_t inLen,
               virt_ptr<void> outBuf,
               uint32_t outLen,
               IOSAsyncCallbackFn callback,
               virt_ptr<void> context);

IOSError
IOS_Ioctlv(IOSHandle handle,
           uint32_t request,
           uint32_t vecIn,
           uint32_t vecOut,
           virt_ptr<IOSVec> vec);

IOSError
IOS_IoctlvAsync(IOSHandle handle,
                uint32_t request,
                uint32_t vecIn,
                uint32_t vecOut,
                virt_ptr<IOSVec> vec,
                IOSAsyncCallbackFn callback,
                virt_ptr<void> context);

/** @} */

} // namespace cafe::coreinit
