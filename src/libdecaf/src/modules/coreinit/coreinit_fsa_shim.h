#pragma once
#include "coreinit_enum.h"
#include "coreinit_ios.h"
#include "coreinit_fsa_request.h"
#include "coreinit_fsa_response.h"
#include "coreinit_messagequeue.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/cbool.h>
#include <common/structsize.h>

namespace coreinit
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
   FSARequest request;
   UNKNOWN(0x60);

   //! Buffer for FSA IPC response.
   FSAResponse response;
   UNKNOWN(0x880 - 0x813);

   //! Memory to use for ioctlv calls, unknown maximum count - but at least 3.
   IOSVec ioctlvVec[3];

   UNKNOWN(0x900 - 0x8A4);

   //! Command for FSA.
   be_val<FSACommand> command;

   //! Handle to FSA device.
   be_val<IOSHandle> clientHandle;

   //! IOS IPC request type to use.
   be_val<FSAIpcRequestType> ipcReqType;

   //! Number of ioctlv input vectors.
   be_val<uint8_t> ioctlvVecIn;

   //! Number of ioctlv output vectors.
   be_val<uint8_t> ioctlvVecOut;

   //! FSAAsyncResult used for FSA* functions.
   FSAAsyncResult fsaAsyncResult;
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
fsaShimSubmitRequest(FSAShimBuffer *shim,
                     FSAStatus emulatedError);

FSAStatus
fsaShimSubmitRequestAsync(FSAShimBuffer *shim,
                          FSAStatus emulatedError,
                          IOSAsyncCallbackFn callback,
                          void *context);

FSAStatus
fsaShimPrepareRequestChangeDir(FSAShimBuffer *shim,
                               IOSHandle clientHandle,
                               const char *path);

FSAStatus
fsaShimPrepareRequestCloseDir(FSAShimBuffer *shim,
                              IOSHandle clientHandle,
                              FSDirHandle dirHandle);

FSAStatus
fsaShimPrepareRequestCloseFile(FSAShimBuffer *shim,
                               IOSHandle clientHandle,
                               FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestFlushFile(FSAShimBuffer *shim,
                               IOSHandle clientHandle,
                               FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestFlushQuota(FSAShimBuffer *shim,
                                IOSHandle clientHandle,
                                const char *path);

FSAStatus
fsaShimPrepareRequestGetCwd(FSAShimBuffer *shim,
                            IOSHandle clientHandle);

FSAStatus
fsaShimPrepareRequestGetInfoByQuery(FSAShimBuffer *shim,
                                    IOSHandle clientHandle,
                                    const char *path,
                                    FSQueryInfoType type);

FSAStatus
fsaShimPrepareRequestGetPosFile(FSAShimBuffer *shim,
                                IOSHandle clientHandle,
                                FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestIsEof(FSAShimBuffer *shim,
                           IOSHandle clientHandle,
                           FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestMakeDir(FSAShimBuffer *shim,
                             IOSHandle clientHandle,
                             const char *path,
                             uint32_t permissions);

FSAStatus
fsaShimPrepareRequestMount(FSAShimBuffer *shim,
                           IOSHandle clientHandle,
                           const char *path,
                           const char *target,
                           uint32_t unk0,
                           void *unkBuf,
                           uint32_t unkBufLen);

FSAStatus
fsaShimPrepareRequestOpenDir(FSAShimBuffer *shim,
                             IOSHandle clientHandle,
                             const char *path);

FSAStatus
fsaShimPrepareRequestOpenFile(FSAShimBuffer *shim,
                              IOSHandle clientHandle,
                              const char *path,
                              const char *mode,
                              uint32_t unk0x290,
                              uint32_t unk0x294,
                              uint32_t unk0x298);

FSAStatus
fsaShimPrepareRequestReadDir(FSAShimBuffer *shim,
                             IOSHandle clientHandle,
                             FSDirHandle dirHandle);

FSAStatus
fsaShimPrepareRequestReadFile(FSAShimBuffer *shim,
                              IOSHandle clientHandle,
                              uint8_t *buffer,
                              uint32_t size,
                              uint32_t count,
                              uint32_t pos,
                              FSFileHandle handle,
                              FSReadFlag readFlags);

FSAStatus
fsaShimPrepareRequestRemove(FSAShimBuffer *shim,
                            IOSHandle clientHandle,
                            const char *path);

FSAStatus
fsaShimPrepareRequestRename(FSAShimBuffer *shim,
                            IOSHandle clientHandle,
                            const char *oldPath,
                            const char *newPath);

FSAStatus
fsaShimPrepareRequestRewindDir(FSAShimBuffer *shim,
                               IOSHandle clientHandle,
                               FSDirHandle dirHandle);

FSAStatus
fsaShimPrepareRequestSetPosFile(FSAShimBuffer *shim,
                                IOSHandle clientHandle,
                                FSFileHandle fileHandle,
                                FSFilePosition pos);

FSAStatus
fsaShimPrepareRequestStatFile(FSAShimBuffer *shim,
                              IOSHandle clientHandle,
                              FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestTruncateFile(FSAShimBuffer *shim,
                                  IOSHandle clientHandle,
                                  FSFileHandle fileHandle);

FSAStatus
fsaShimPrepareRequestUnmount(FSAShimBuffer *shim,
                             IOSHandle clientHandle,
                             const char *path,
                             uint32_t unk0x280);

FSAStatus
fsaShimPrepareRequestWriteFile(FSAShimBuffer *shim,
                               IOSHandle clientHandle,
                               const uint8_t *buffer,
                               uint32_t size,
                               uint32_t count,
                               uint32_t pos,
                               FSFileHandle handle,
                               FSWriteFlag writeFlags);

} // namespace internal

/** @} */

} // namespace coreinit
