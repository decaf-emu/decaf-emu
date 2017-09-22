#include "coreinit.h"
#include "coreinit_fsa_shim.h"
#include "ios/ios_error.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>

namespace coreinit
{

/*
Unimplemented FSA Shim functions:
__FSAShimAllocateBuffer
__FSAShimFreeBuffer
__FSAShimCheckClientHandle
*/

/**
 * Using the power of magic, turns an IOSError into an FSAStatus.
 */
FSAStatus
FSAShimDecodeIosErrorToFsaStatus(IOSHandle handle,
                                 IOSError error)
{
   auto category = ios::getErrorCategory(error);
   auto code = ios::getErrorCode(error);
   auto fsaStatus = static_cast<FSAStatus>(error);

   if (error < 0) {
      switch (category) {
      case IOSErrorCategory::Kernel:
         if (code == IOSError::Access) {
            fsaStatus = FSAStatus::InvalidBuffer;
         } else if (code == IOSError::Invalid || code == IOSError::NoExists) {
            fsaStatus = FSAStatus::InvalidClientHandle;
         } else if (code == IOSError::QFull) {
            fsaStatus = FSAStatus::Busy;
         } else {
            fsaStatus = static_cast<FSAStatus>(code);
         }
         break;
      case IOSErrorCategory::FSA:
      case IOSErrorCategory::Unknown7:
      case IOSErrorCategory::Unknown8:
      case IOSErrorCategory::Unknown15:
      case IOSErrorCategory::Unknown19:
      case IOSErrorCategory::Unknown30:
      case IOSErrorCategory::Unknown45:
         if (ios::isKernelError(code)) {
            fsaStatus = static_cast<FSAStatus>(code - (IOSErrorCategory::FSA << 16));
         } else {
            fsaStatus = static_cast<FSAStatus>(code);
         }
         break;
      }
   }

   return fsaStatus;
}


namespace internal
{

/**
 * Open FSA device.
 */
IOSError
fsaShimOpen()
{
   return IOS_Open("/dev/fsa", IOSOpenMode::None);
}


/**
 * Close FSA device.
 */
IOSError
fsaShimClose(IOSHandle handle)
{
   return IOS_Close(handle);
}


/**
 * Submit a synchronous FSA request.
 */
FSAStatus
fsaShimSubmitRequest(FSAShimBuffer *shim,
                     FSAStatus emulatedError)
{
   auto iosError = IOSError::Invalid;

   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->request.emulatedError = emulatedError;

   if (shim->ipcReqType == FSAIpcRequestType::Ioctl) {
      iosError = IOS_Ioctl(shim->clientHandle,
                           shim->command,
                           &shim->request,
                           sizeof(shim->request),
                           &shim->response,
                           sizeof(shim->response));
   } else if (shim->ipcReqType == FSAIpcRequestType::Ioctlv) {
      iosError = IOS_Ioctlv(shim->clientHandle,
                            shim->command,
                            shim->ioctlvVecIn,
                            shim->ioctlvVecOut,
                            shim->ioctlvVec);
   } else {
      decaf_abort(fmt::format("Invalid reqType {}", shim->ipcReqType));
   }

   return FSAShimDecodeIosErrorToFsaStatus(shim->clientHandle, iosError);
}


/**
 * Submit an asynchronous FSA request.
 */
FSAStatus
fsaShimSubmitRequestAsync(FSAShimBuffer *shim,
                          FSAStatus emulatedError,
                          IOSAsyncCallbackFn callback,
                          void *context)
{
   auto iosError = IOSError::Invalid;

   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->request.emulatedError = emulatedError;

   if (shim->ipcReqType == FSAIpcRequestType::Ioctl) {
      iosError = IOS_IoctlAsync(shim->clientHandle,
                                shim->command,
                                &shim->request,
                                sizeof(shim->request),
                                &shim->response,
                                sizeof(shim->response),
                                callback,
                                context);
   } else if (shim->ipcReqType == FSAIpcRequestType::Ioctlv) {
      iosError = IOS_IoctlvAsync(shim->clientHandle,
                                 shim->command,
                                 shim->ioctlvVecIn,
                                 shim->ioctlvVecOut,
                                 shim->ioctlvVec,
                                 callback,
                                 context);
   } else {
      decaf_abort(fmt::format("Invalid reqType {}", shim->ipcReqType));
   }

   return FSAShimDecodeIosErrorToFsaStatus(shim->clientHandle, iosError);
}


/**
 * Prepare a FSACommand::ChangeDir request.
 */
FSAStatus
fsaShimPrepareRequestChangeDir(FSAShimBuffer *shim,
                               IOSHandle clientHandle,
                               const char *path)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || strlen(path) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::ChangeDir;

   auto request = &shim->request.changeDir;
   std::strncpy(request->path, path, FSMaxPathLength);

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::CloseDir request.
 */
FSAStatus
fsaShimPrepareRequestCloseDir(FSAShimBuffer *shim,
                              IOSHandle clientHandle,
                              FSDirHandle dirHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::CloseDir;

   auto request = &shim->request.closeDir;
   request->handle = dirHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::CloseFile request.
 */
FSAStatus
fsaShimPrepareRequestCloseFile(FSAShimBuffer *shim,
                               IOSHandle clientHandle,
                               FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::CloseFile;

   auto request = &shim->request.closeFile;
   request->handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::FlushFile request.
 */
FSAStatus
fsaShimPrepareRequestFlushFile(FSAShimBuffer *shim,
                               IOSHandle clientHandle,
                               FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::FlushFile;

   auto request = &shim->request.flushFile;
   request->handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::FlushQuota request.
 */
FSAStatus
fsaShimPrepareRequestFlushQuota(FSAShimBuffer *shim,
                                IOSHandle clientHandle,
                                const char *path)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || strlen(path) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::FlushQuota;

   auto request = &shim->request.flushQuota;
   std::strncpy(request->path, path, FSMaxPathLength);

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::GetCwd request.
 */
FSAStatus
fsaShimPrepareRequestGetCwd(FSAShimBuffer *shim,
                            IOSHandle clientHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::GetCwd;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::GetInfoByQuery request.
 */
FSAStatus
fsaShimPrepareRequestGetInfoByQuery(FSAShimBuffer *shim,
                                    IOSHandle clientHandle,
                                    const char *path,
                                    FSAQueryInfoType type)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || strlen(path) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   if (type > FSAQueryInfoType::FragmentBlockInfo) {
      return FSAStatus::InvalidParam;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::GetInfoByQuery;

   auto request = &shim->request.getInfoByQuery;
   std::strncpy(request->path, path, FSMaxPathLength);
   request->type = type;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::GetPosFile request.
 */
FSAStatus
fsaShimPrepareRequestGetPosFile(FSAShimBuffer *shim,
                                IOSHandle clientHandle,
                                FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::GetPosFile;

   auto request = &shim->request.getPosFile;
   request->handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::IsEof request.
 */
FSAStatus
fsaShimPrepareRequestIsEof(FSAShimBuffer *shim,
                           IOSHandle clientHandle,
                           FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::IsEof;

   auto request = &shim->request.isEof;
   request->handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::MakeDir request.
 */
FSAStatus
fsaShimPrepareRequestMakeDir(FSAShimBuffer *shim,
                             IOSHandle clientHandle,
                             const char *path,
                             uint32_t permissions)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || strlen(path) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::MakeDir;

   auto request = &shim->request.makeDir;
   std::strncpy(request->path, path, FSMaxPathLength);
   request->permission = permissions;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::Mount request.
 */
FSAStatus
fsaShimPrepareRequestMount(FSAShimBuffer *shim,
                           IOSHandle clientHandle,
                           const char *path,
                           const char *target,
                           uint32_t unk0,
                           void *unkBuf,
                           uint32_t unkBufLen)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || strlen(path) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctlv;
   shim->command = FSACommand::Mount;

   auto request = &shim->request.mount;
   std::strncpy(request->path, path, FSMaxPathLength);
   std::strncpy(request->target, target, FSMaxPathLength);
   request->unk0x500 = unk0;
   request->unk0x508 = unkBufLen;

   shim->ioctlvVecIn = 2;
   shim->ioctlvVecOut = 1;

   shim->ioctlvVec[0].vaddr = cpu::translate(&shim->request);
   shim->ioctlvVec[0].len = static_cast<uint32_t>(sizeof(FSARequest));

   shim->ioctlvVec[1].vaddr = cpu::translate(unkBuf);
   shim->ioctlvVec[1].len = unkBufLen;

   shim->ioctlvVec[2].vaddr = cpu::translate(&shim->response);
   shim->ioctlvVec[2].len = static_cast<uint32_t>(sizeof(FSAResponse));

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::OpenFile request.
 */
FSAStatus
fsaShimPrepareRequestOpenDir(FSAShimBuffer *shim,
                             IOSHandle clientHandle,
                             const char *path)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || strlen(path) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::OpenDir;

   auto request = &shim->request.openDir;
   std::strncpy(request->path, path, FSMaxPathLength);

   auto response = &shim->response.openDir;
   response->handle = -1;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::OpenFile request.
 */
FSAStatus
fsaShimPrepareRequestOpenFile(FSAShimBuffer *shim,
                              IOSHandle clientHandle,
                              const char *path,
                              const char *mode,
                              uint32_t unk0x290,
                              uint32_t unk0x294,
                              uint32_t unk0x298)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || strlen(path) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   if (!mode || strlen(mode) >= 15) {
      return FSAStatus::InvalidParam;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::OpenFile;

   auto request = &shim->request.openFile;
   std::strncpy(request->path, path, FSMaxPathLength);
   std::strncpy(request->mode, mode, 16);
   request->unk0x290 = unk0x290;
   request->unk0x294 = unk0x294;
   request->unk0x298 = unk0x298;

   auto response = &shim->response.openFile;
   response->handle = -1;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::ReadDir request.
 */
FSAStatus
fsaShimPrepareRequestReadDir(FSAShimBuffer *shim,
                             IOSHandle clientHandle,
                             FSDirHandle dirHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::ReadDir;

   auto request = &shim->request.readDir;
   request->handle = dirHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::ReadFile request.
 */
FSAStatus
fsaShimPrepareRequestReadFile(FSAShimBuffer *shim,
                              IOSHandle clientHandle,
                              uint8_t *buffer,
                              uint32_t size,
                              uint32_t count,
                              uint32_t pos,
                              FSFileHandle handle,
                              FSAReadFlag readFlags)
{
   if (!shim || !buffer) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctlv;
   shim->command = FSACommand::ReadFile;

   shim->ioctlvVecIn = 1;
   shim->ioctlvVecOut = 2;

   shim->ioctlvVec[0].vaddr = cpu::translate(&shim->request);
   shim->ioctlvVec[0].len = static_cast<uint32_t>(sizeof(FSARequest));

   shim->ioctlvVec[1].vaddr = cpu::translate(buffer);
   shim->ioctlvVec[1].len = size * count;

   shim->ioctlvVec[2].vaddr = cpu::translate(&shim->response);
   shim->ioctlvVec[2].len = static_cast<uint32_t>(sizeof(FSAResponse));

   auto request = &shim->request.readFile;
   request->buffer = buffer;
   request->size = size;
   request->count = count;
   request->pos = pos;
   request->handle = handle;
   request->readFlags = readFlags;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::Remove request.
 */
FSAStatus
fsaShimPrepareRequestRemove(FSAShimBuffer *shim,
                            IOSHandle clientHandle,
                            const char *path)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || strlen(path) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::Remove;

   auto request = &shim->request.remove;
   std::strncpy(request->path, path, FSMaxPathLength);

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::Rename request.
 */
FSAStatus
fsaShimPrepareRequestRename(FSAShimBuffer *shim,
                            IOSHandle clientHandle,
                            const char *oldPath,
                            const char *newPath)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!oldPath || strlen(oldPath) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   if (!newPath || strlen(newPath) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::Rename;

   auto request = &shim->request.rename;
   std::strncpy(request->oldPath, oldPath, FSMaxPathLength);
   std::strncpy(request->newPath, newPath, FSMaxPathLength);

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::RewindDir request.
 */
FSAStatus
fsaShimPrepareRequestRewindDir(FSAShimBuffer *shim,
                               IOSHandle clientHandle,
                               FSDirHandle dirHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::RewindDir;

   auto request = &shim->request.rewindDir;
   request->handle = dirHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::SetPosFile request.
 */
FSAStatus
fsaShimPrepareRequestSetPosFile(FSAShimBuffer *shim,
                                IOSHandle clientHandle,
                                FSFileHandle fileHandle,
                                FSFilePosition pos)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::SetPosFile;

   auto request = &shim->request.setPosFile;
   request->handle = fileHandle;
   request->pos = pos;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::StatFile request.
 */
FSAStatus
fsaShimPrepareRequestStatFile(FSAShimBuffer *shim,
                              IOSHandle clientHandle,
                              FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::StatFile;

   auto request = &shim->request.statFile;
   request->handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::TruncateFile request.
 */
FSAStatus
fsaShimPrepareRequestTruncateFile(FSAShimBuffer *shim,
                                  IOSHandle clientHandle,
                                  FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::TruncateFile;

   auto request = &shim->request.truncateFile;
   request->handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::Unmount request.
 */
FSAStatus
fsaShimPrepareRequestUnmount(FSAShimBuffer *shim,
                             IOSHandle clientHandle,
                             const char *path,
                             uint32_t unk0x280)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || strlen(path) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::Unmount;

   auto request = &shim->request.unmount;
   std::strncpy(request->path, path, FSMaxPathLength);
   request->unk0x280 = unk0x280;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::WriteFile request.
 */
FSAStatus
fsaShimPrepareRequestWriteFile(FSAShimBuffer *shim,
                               IOSHandle clientHandle,
                               const uint8_t *buffer,
                               uint32_t size,
                               uint32_t count,
                               uint32_t pos,
                               FSFileHandle handle,
                               FSAWriteFlag writeFlags)
{
   if (!shim || !buffer) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctlv;
   shim->command = FSACommand::WriteFile;

   shim->ioctlvVecIn = 2;
   shim->ioctlvVecOut = 1;

   shim->ioctlvVec[0].vaddr = cpu::translate(&shim->request);
   shim->ioctlvVec[0].len = static_cast<uint32_t>(sizeof(FSARequest));

   shim->ioctlvVec[1].vaddr = cpu::translate(buffer);
   shim->ioctlvVec[1].len = size * count;

   shim->ioctlvVec[2].vaddr = cpu::translate(&shim->response);
   shim->ioctlvVec[2].len = static_cast<uint32_t>(sizeof(FSAResponse));

   auto request = &shim->request.writeFile;
   request->buffer = buffer;
   request->size = size;
   request->count = count;
   request->pos = pos;
   request->handle = handle;
   request->writeFlags = writeFlags;

   return FSAStatus::OK;
}

} // namespace internal

void
Module::registerFsaShimFunctions()
{
   RegisterKernelFunctionName("__FSAShimDecodeIosErrorToFsaStatus", FSAShimDecodeIosErrorToFsaStatus);
}

} // namespace coreinit
