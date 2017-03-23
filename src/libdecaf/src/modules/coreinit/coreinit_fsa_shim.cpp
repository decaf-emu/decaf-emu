#include "coreinit.h"
#include "coreinit_fsa_shim.h"

#include <common/decaf_assert.h>
#include <spdlog/fmt/fmt.h>

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
   auto category = ((~error) >> 16) & 0x3FF;

   if (error >= 0 || category == 0x3) {
      return static_cast<FSAStatus>(error);
   } else {
      auto status = static_cast<FSAStatus>(0xFFFF0000 | error);
      return status;
   }
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
                     uint32_t requestWord0)
{
   auto iosError = IOSError::Invalid;

   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->request.unk0x00 = requestWord0;

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
                          uint32_t requestWord0,
                          IOSAsyncCallbackFn callback,
                          void *context)
{
   auto iosError = IOSError::Invalid;

   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->request.unk0x00 = requestWord0;

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
                              FSReadFlag readFlags)
{
   if (!shim || !buffer) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctlv;
   shim->command = FSACommand::ReadFile;

   shim->ioctlvVecIn = 1;
   shim->ioctlvVecOut = 2;

   shim->ioctlvVec[0].paddr = &shim->request;
   shim->ioctlvVec[0].len = sizeof(FSARequest);

   shim->ioctlvVec[1].paddr = buffer;
   shim->ioctlvVec[1].len = size * count;

   shim->ioctlvVec[2].paddr = &shim->response;
   shim->ioctlvVec[2].len = sizeof(FSAResponse);

   auto request = &shim->request.readFile;
   request->buffer = buffer;
   request->size = size;
   request->count = count;
   request->pos = pos;
   request->handle = handle;
   request->readFlags = readFlags;

   return FSAStatus::OK;
}

} // namespace internal

void
Module::registerFsaShimFunctions()
{
   RegisterKernelFunctionName("__FSAShimDecodeIosErrorToFsaStatus", FSAShimDecodeIosErrorToFsaStatus);
}

} // namespace coreinit
