#pragma once
#include "coreinit_enum.h"
#include "coreinit_fs.h"
#include "coreinit_fsa.h"
#include "coreinit_ios.h"
#include "coreinit_messagequeue.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \ingroup coreinit_fsa
 * @{
 */

#pragma pack(push, 1)

struct FSAShimBuffer;

/**
 * Stores data regarding FSA IPC requests.
 */
struct FSAShimBuffer
{
   //! Buffer for FSA IPC request.
   be2_struct<FSARequest> request;
   UNKNOWN(0x60);

   //! Buffer for FSA IPC response.
   be2_struct<FSAResponse> response;
   UNKNOWN(0x880 - 0x813);

   //! Memory to use for ioctlv calls, unknown maximum count - but at least 3.
   be2_array<IOSVec, 3> ioctlvVec;

   UNKNOWN(0x900 - 0x8A4);

   //! Command for FSA.
   be2_val<FSACommand> command;

   //! Handle to FSA device.
   be2_val<IOSHandle> clientHandle;

   //! IOS IPC request type to use.
   be2_val<FSAIpcRequestType> ipcReqType;

   //! Number of ioctlv input vectors.
   be2_val<uint8_t> ioctlvVecIn;

   //! Number of ioctlv output vectors.
   be2_val<uint8_t> ioctlvVecOut;

   //! FSAAsyncResult used for FSA* functions.
   be2_struct<FSAAsyncResult> fsaAsyncResult;
};
CHECK_OFFSET(FSAShimBuffer, 0x0, request);
CHECK_OFFSET(FSAShimBuffer, 0x580, response);
CHECK_OFFSET(FSAShimBuffer, 0x880, ioctlvVec);
CHECK_OFFSET(FSAShimBuffer, 0x900, command);
CHECK_OFFSET(FSAShimBuffer, 0x904, clientHandle);
CHECK_OFFSET(FSAShimBuffer, 0x908, ipcReqType);
CHECK_OFFSET(FSAShimBuffer, 0x90A, ioctlvVecIn);
CHECK_OFFSET(FSAShimBuffer, 0x90B, ioctlvVecOut);
CHECK_OFFSET(FSAShimBuffer, 0x90C, fsaAsyncResult);
CHECK_SIZE(FSAShimBuffer, 0x938);

#pragma pack(pop)

FSAStatus
FSAShimDecodeIosErrorToFsaStatus(IOSHandle handle,
                                 IOSError error);

namespace internal
{

IOSError
fsaShimOpen();

IOSError
fsaShimClose(IOSHandle handle);

FSAStatus
fsaShimSubmitRequest(virt_ptr<FSAShimBuffer> shim,
                     FSAStatus emulatedError);

FSAStatus
fsaShimSubmitRequestAsync(virt_ptr<FSAShimBuffer> shim,
                          FSAStatus emulatedError,
                          IOSAsyncCallbackFn callback,
                          virt_ptr<void> context);

FSAStatus
fsaShimPrepareRequestChangeDir(virt_ptr<FSAShimBuffer> shim,
                               IOSHandle clientHandle,
                               virt_ptr<const char> path);

FSAStatus
fsaShimPrepareRequestCloseDir(virt_ptr<FSAShimBuffer> shim,
                              IOSHandle clientHandle,
                              FSDirHandle dirHandle);

FSAStatus
fsaShimPrepareRequestCloseFile(virt_ptr<FSAShimBuffer> shim,
                               IOSHandle clientHandle,
                               FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestFlushFile(virt_ptr<FSAShimBuffer> shim,
                               IOSHandle clientHandle,
                               FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestFlushQuota(virt_ptr<FSAShimBuffer> shim,
                                IOSHandle clientHandle,
                                virt_ptr<const char> path);

FSAStatus
fsaShimPrepareRequestGetCwd(virt_ptr<FSAShimBuffer> shim,
                            IOSHandle clientHandle);

FSAStatus
fsaShimPrepareRequestGetInfoByQuery(virt_ptr<FSAShimBuffer> shim,
                                    IOSHandle clientHandle,
                                    virt_ptr<const char> path,
                                    FSAQueryInfoType type);

FSAStatus
fsaShimPrepareRequestGetPosFile(virt_ptr<FSAShimBuffer> shim,
                                IOSHandle clientHandle,
                                FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestIsEof(virt_ptr<FSAShimBuffer> shim,
                           IOSHandle clientHandle,
                           FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestMakeDir(virt_ptr<FSAShimBuffer> shim,
                             IOSHandle clientHandle,
                             virt_ptr<const char> path,
                             uint32_t permissions);

FSAStatus
fsaShimPrepareRequestMount(virt_ptr<FSAShimBuffer> shim,
                           IOSHandle clientHandle,
                           virt_ptr<const char> path,
                           virt_ptr<const char> target,
                           uint32_t unk0,
                           virt_ptr<void> unkBuf,
                           uint32_t unkBufLen);

FSAStatus
fsaShimPrepareRequestOpenDir(virt_ptr<FSAShimBuffer> shim,
                             IOSHandle clientHandle,
                             virt_ptr<const char> path);

FSAStatus
fsaShimPrepareRequestOpenFile(virt_ptr<FSAShimBuffer> shim,
                              IOSHandle clientHandle,
                              virt_ptr<const char> path,
                              virt_ptr<const char> mode,
                              uint32_t unk0x290,
                              uint32_t unk0x294,
                              uint32_t unk0x298);

FSAStatus
fsaShimPrepareRequestReadDir(virt_ptr<FSAShimBuffer> shim,
                             IOSHandle clientHandle,
                             FSDirHandle dirHandle);

FSAStatus
fsaShimPrepareRequestReadFile(virt_ptr<FSAShimBuffer> shim,
                              IOSHandle clientHandle,
                              virt_ptr<uint8_t> buffer,
                              uint32_t size,
                              uint32_t count,
                              uint32_t pos,
                              FSFileHandle handle,
                              FSAReadFlag readFlags);

FSAStatus
fsaShimPrepareRequestRemove(virt_ptr<FSAShimBuffer> shim,
                            IOSHandle clientHandle,
                            virt_ptr<const char> path);

FSAStatus
fsaShimPrepareRequestRename(virt_ptr<FSAShimBuffer> shim,
                            IOSHandle clientHandle,
                            virt_ptr<const char> oldPath,
                            virt_ptr<const char> newPath);

FSAStatus
fsaShimPrepareRequestRewindDir(virt_ptr<FSAShimBuffer> shim,
                               IOSHandle clientHandle,
                               FSDirHandle dirHandle);

FSAStatus
fsaShimPrepareRequestSetPosFile(virt_ptr<FSAShimBuffer> shim,
                                IOSHandle clientHandle,
                                FSFileHandle fileHandle,
                                FSFilePosition pos);

FSAStatus
fsaShimPrepareRequestStatFile(virt_ptr<FSAShimBuffer> shim,
                              IOSHandle clientHandle,
                              FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestTruncateFile(virt_ptr<FSAShimBuffer> shim,
                                  IOSHandle clientHandle,
                                  FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestUnmount(virt_ptr<FSAShimBuffer> shim,
                             IOSHandle clientHandle,
                             virt_ptr<const char> path,
                             uint32_t unk0x280);

FSAStatus
fsaShimPrepareRequestWriteFile(virt_ptr<FSAShimBuffer> shim,
                               IOSHandle clientHandle,
                               virt_ptr<const uint8_t> buffer,
                               uint32_t size,
                               uint32_t count,
                               uint32_t pos,
                               FSFileHandle handle,
                               FSAWriteFlag writeFlags);

} // namespace internal

/** @} */

} // namespace cafe::coreinit
