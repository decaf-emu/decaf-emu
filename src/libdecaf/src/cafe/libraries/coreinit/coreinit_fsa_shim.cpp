#include "coreinit.h"
#include "coreinit_fsa_shim.h"
#include "cafe/cafe_stackobject.h"
#include "ios/ios_error.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>

namespace cafe::coreinit
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
   return IOS_Open(make_stack_string("/dev/fsa"), IOSOpenMode::None);
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
fsaShimSubmitRequest(virt_ptr<FSAShimBuffer> shim,
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
                           virt_addrof(shim->request),
                           sizeof(shim->request),
                           virt_addrof(shim->response),
                           sizeof(shim->response));
   } else if (shim->ipcReqType == FSAIpcRequestType::Ioctlv) {
      iosError = IOS_Ioctlv(shim->clientHandle,
                            shim->command,
                            shim->ioctlvVecIn,
                            shim->ioctlvVecOut,
                            virt_addrof(shim->ioctlvVec));
   } else {
      decaf_abort(fmt::format("Invalid reqType {}", shim->ipcReqType));
   }

   return FSAShimDecodeIosErrorToFsaStatus(shim->clientHandle, iosError);
}


/**
 * Submit an asynchronous FSA request.
 */
FSAStatus
fsaShimSubmitRequestAsync(virt_ptr<FSAShimBuffer> shim,
                          FSAStatus emulatedError,
                          IOSAsyncCallbackFn callback,
                          virt_ptr<void> context)
{
   auto iosError = IOSError::Invalid;

   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->request.emulatedError = emulatedError;

   if (shim->ipcReqType == FSAIpcRequestType::Ioctl) {
      iosError = IOS_IoctlAsync(shim->clientHandle,
                                shim->command,
                                virt_addrof(shim->request),
                                sizeof(shim->request),
                                virt_addrof(shim->response),
                                sizeof(shim->response),
                                callback,
                                context);
   } else if (shim->ipcReqType == FSAIpcRequestType::Ioctlv) {
      iosError = IOS_IoctlvAsync(shim->clientHandle,
                                 shim->command,
                                 shim->ioctlvVecIn,
                                 shim->ioctlvVecOut,
                                 virt_addrof(shim->ioctlvVec),
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
fsaShimPrepareRequestChangeDir(virt_ptr<FSAShimBuffer> shim,
                               IOSHandle clientHandle,
                               virt_ptr<const char> path)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || std::strlen(path.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::ChangeDir;

   auto request = virt_addrof(shim->request.changeDir);
   std::strncpy(virt_addrof(request->path).getRawPointer(),
                path.getRawPointer(),
                FSMaxPathLength);

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::CloseDir request.
 */
FSAStatus
fsaShimPrepareRequestCloseDir(virt_ptr<FSAShimBuffer> shim,
                              IOSHandle clientHandle,
                              FSDirHandle dirHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::CloseDir;

   auto request = virt_addrof(shim->request.closeDir);
   request->handle = dirHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::CloseFile request.
 */
FSAStatus
fsaShimPrepareRequestCloseFile(virt_ptr<FSAShimBuffer> shim,
                               IOSHandle clientHandle,
                               FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::CloseFile;

   auto request = virt_addrof(shim->request.closeFile);
   request->handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::FlushFile request.
 */
FSAStatus
fsaShimPrepareRequestFlushFile(virt_ptr<FSAShimBuffer> shim,
                               IOSHandle clientHandle,
                               FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::FlushFile;

   auto request = virt_addrof(shim->request.flushFile);
   request->handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::FlushQuota request.
 */
FSAStatus
fsaShimPrepareRequestFlushQuota(virt_ptr<FSAShimBuffer> shim,
                                IOSHandle clientHandle,
                                virt_ptr<const char> path)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || std::strlen(path.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::FlushQuota;

   auto request = virt_addrof(shim->request.flushQuota);
   std::strncpy(virt_addrof(request->path).getRawPointer(),
                path.getRawPointer(),
                FSMaxPathLength);

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::GetCwd request.
 */
FSAStatus
fsaShimPrepareRequestGetCwd(virt_ptr<FSAShimBuffer> shim,
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
fsaShimPrepareRequestGetInfoByQuery(virt_ptr<FSAShimBuffer> shim,
                                    IOSHandle clientHandle,
                                    virt_ptr<const char> path,
                                    FSAQueryInfoType type)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || std::strlen(path.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   if (type > FSAQueryInfoType::FragmentBlockInfo) {
      return FSAStatus::InvalidParam;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::GetInfoByQuery;

   auto request = virt_addrof(shim->request.getInfoByQuery);
   std::strncpy(virt_addrof(request->path).getRawPointer(),
                path.getRawPointer(),
                FSMaxPathLength);
   request->type = type;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::GetPosFile request.
 */
FSAStatus
fsaShimPrepareRequestGetPosFile(virt_ptr<FSAShimBuffer> shim,
                                IOSHandle clientHandle,
                                FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::GetPosFile;

   auto request = virt_addrof(shim->request.getPosFile);
   request->handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::IsEof request.
 */
FSAStatus
fsaShimPrepareRequestIsEof(virt_ptr<FSAShimBuffer> shim,
                           IOSHandle clientHandle,
                           FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::IsEof;

   auto request = virt_addrof(shim->request.isEof);
   request->handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::MakeDir request.
 */
FSAStatus
fsaShimPrepareRequestMakeDir(virt_ptr<FSAShimBuffer> shim,
                             IOSHandle clientHandle,
                             virt_ptr<const char> path,
                             uint32_t permissions)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || std::strlen(path.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::MakeDir;

   auto request = virt_addrof(shim->request.makeDir);
   std::strncpy(virt_addrof(request->path).getRawPointer(),
                path.getRawPointer(),
                FSMaxPathLength);
   request->permission = permissions;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::Mount request.
 */
FSAStatus
fsaShimPrepareRequestMount(virt_ptr<FSAShimBuffer> shim,
                           IOSHandle clientHandle,
                           virt_ptr<const char> path,
                           virt_ptr<const char> target,
                           uint32_t unk0,
                           virt_ptr<void> unkBuf,
                           uint32_t unkBufLen)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || std::strlen(path.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctlv;
   shim->command = FSACommand::Mount;

   auto request = virt_addrof(shim->request.mount);
   std::strncpy(virt_addrof(request->path).getRawPointer(),
                path.getRawPointer(),
                FSMaxPathLength);
   std::strncpy(virt_addrof(request->target).getRawPointer(),
                target.getRawPointer(),
                FSMaxPathLength);
   request->unk0x500 = unk0;
   request->unkBufLen = unkBufLen;

   shim->ioctlvVecIn = uint8_t { 2 };
   shim->ioctlvVecOut = uint8_t { 1 };

   shim->ioctlvVec[0].vaddr = virt_cast<virt_addr>(virt_addrof(shim->request));
   shim->ioctlvVec[0].len = static_cast<uint32_t>(sizeof(FSARequest));

   shim->ioctlvVec[1].vaddr = virt_cast<virt_addr>(unkBuf);
   shim->ioctlvVec[1].len = unkBufLen;

   shim->ioctlvVec[2].vaddr = virt_cast<virt_addr>(virt_addrof(shim->response));
   shim->ioctlvVec[2].len = static_cast<uint32_t>(sizeof(FSAResponse));

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::OpenFile request.
 */
FSAStatus
fsaShimPrepareRequestOpenDir(virt_ptr<FSAShimBuffer> shim,
                             IOSHandle clientHandle,
                             virt_ptr<const char> path)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || std::strlen(path.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::OpenDir;

   auto request = virt_addrof(shim->request.openDir);
   std::strncpy(virt_addrof(request->path).getRawPointer(),
                path.getRawPointer(),
                FSMaxPathLength);

   auto response = virt_addrof(shim->response.openDir);
   response->handle = -1;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::OpenFile request.
 */
FSAStatus
fsaShimPrepareRequestOpenFile(virt_ptr<FSAShimBuffer> shim,
                              IOSHandle clientHandle,
                              virt_ptr<const char> path,
                              virt_ptr<const char> mode,
                              uint32_t unk0x290,
                              uint32_t unk0x294,
                              uint32_t unk0x298)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || std::strlen(path.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   if (!mode || std::strlen(mode.getRawPointer()) >= 15) {
      return FSAStatus::InvalidParam;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::OpenFile;

   auto &request = shim->request.openFile;
   std::strncpy(virt_addrof(request.path).getRawPointer(),
                path.getRawPointer(),
                FSMaxPathLength);
   std::strncpy(virt_addrof(request.mode).getRawPointer(),
                mode.getRawPointer(),
                16);
   request.unk0x290 = unk0x290;
   request.unk0x294 = unk0x294;
   request.unk0x298 = unk0x298;

   auto &response = shim->response.openFile;
   response.handle = -1;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::ReadDir request.
 */
FSAStatus
fsaShimPrepareRequestReadDir(virt_ptr<FSAShimBuffer> shim,
                             IOSHandle clientHandle,
                             FSDirHandle dirHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::ReadDir;

   auto &request = shim->request.readDir;
   request.handle = dirHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::ReadFile request.
 */
FSAStatus
fsaShimPrepareRequestReadFile(virt_ptr<FSAShimBuffer> shim,
                              IOSHandle clientHandle,
                              virt_ptr<uint8_t> buffer,
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

   shim->ioctlvVecIn = uint8_t { 1 };
   shim->ioctlvVecOut = uint8_t { 2 };

   shim->ioctlvVec[0].vaddr = virt_cast<virt_addr>(virt_addrof(shim->request));
   shim->ioctlvVec[0].len = static_cast<uint32_t>(sizeof(FSARequest));

   shim->ioctlvVec[1].vaddr = virt_cast<virt_addr>(buffer);
   shim->ioctlvVec[1].len = size * count;

   shim->ioctlvVec[2].vaddr = virt_cast<virt_addr>(virt_addrof(shim->response));
   shim->ioctlvVec[2].len = static_cast<uint32_t>(sizeof(FSAResponse));

   auto &request = shim->request.readFile;
   request.buffer = virt_cast<uint8_t *>(buffer);
   request.size = size;
   request.count = count;
   request.pos = pos;
   request.handle = handle;
   request.readFlags = readFlags;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::Remove request.
 */
FSAStatus
fsaShimPrepareRequestRemove(virt_ptr<FSAShimBuffer> shim,
                            IOSHandle clientHandle,
                            virt_ptr<const char> path)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || std::strlen(path.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::Remove;

   auto &request = shim->request.remove;
   std::strncpy(virt_addrof(request.path).getRawPointer(),
                path.getRawPointer(),
                FSMaxPathLength);

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::Rename request.
 */
FSAStatus
fsaShimPrepareRequestRename(virt_ptr<FSAShimBuffer> shim,
                            IOSHandle clientHandle,
                            virt_ptr<const char> oldPath,
                            virt_ptr<const char> newPath)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!oldPath || std::strlen(oldPath.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   if (!newPath || std::strlen(newPath.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::Rename;

   auto &request =shim->request.rename;
   std::strncpy(virt_addrof(request.oldPath).getRawPointer(),
                oldPath.getRawPointer(),
                FSMaxPathLength);
   std::strncpy(virt_addrof(request.newPath).getRawPointer(),
                newPath.getRawPointer(),
                FSMaxPathLength);

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::RewindDir request.
 */
FSAStatus
fsaShimPrepareRequestRewindDir(virt_ptr<FSAShimBuffer> shim,
                               IOSHandle clientHandle,
                               FSDirHandle dirHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::RewindDir;

   auto &request = shim->request.rewindDir;
   request.handle = dirHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::SetPosFile request.
 */
FSAStatus
fsaShimPrepareRequestSetPosFile(virt_ptr<FSAShimBuffer> shim,
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

   auto &request = shim->request.setPosFile;
   request.handle = fileHandle;
   request.pos = pos;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::StatFile request.
 */
FSAStatus
fsaShimPrepareRequestStatFile(virt_ptr<FSAShimBuffer> shim,
                              IOSHandle clientHandle,
                              FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::StatFile;

   auto &request = shim->request.statFile;
   request.handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::TruncateFile request.
 */
FSAStatus
fsaShimPrepareRequestTruncateFile(virt_ptr<FSAShimBuffer> shim,
                                  IOSHandle clientHandle,
                                  FSFileHandle fileHandle)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::TruncateFile;

   auto &request = shim->request.truncateFile;
   request.handle = fileHandle;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::Unmount request.
 */
FSAStatus
fsaShimPrepareRequestUnmount(virt_ptr<FSAShimBuffer> shim,
                             IOSHandle clientHandle,
                             virt_ptr<const char> path,
                             uint32_t unk0x280)
{
   if (!shim) {
      return FSAStatus::InvalidBuffer;
   }

   if (!path || std::strlen(path.getRawPointer()) >= FSMaxPathLength) {
      return FSAStatus::InvalidPath;
   }

   shim->clientHandle = clientHandle;
   shim->ipcReqType = FSAIpcRequestType::Ioctl;
   shim->command = FSACommand::Unmount;

   auto &request = shim->request.unmount;
   std::strncpy(virt_addrof(request.path).getRawPointer(),
                path.getRawPointer(),
                FSMaxPathLength);
   request.unk0x280 = unk0x280;

   return FSAStatus::OK;
}


/**
 * Prepare a FSACommand::WriteFile request.
 */
FSAStatus
fsaShimPrepareRequestWriteFile(virt_ptr<FSAShimBuffer> shim,
                               IOSHandle clientHandle,
                               virt_ptr<const uint8_t> buffer,
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

   shim->ioctlvVecIn = uint8_t { 2 };
   shim->ioctlvVecOut = uint8_t { 1 };

   shim->ioctlvVec[0].vaddr = virt_cast<virt_addr>(virt_addrof(shim->request));
   shim->ioctlvVec[0].len = static_cast<uint32_t>(sizeof(FSARequest));

   shim->ioctlvVec[1].vaddr = virt_cast<virt_addr>(buffer);
   shim->ioctlvVec[1].len = size * count;

   shim->ioctlvVec[2].vaddr = virt_cast<virt_addr>(virt_addrof(shim->response));
   shim->ioctlvVec[2].len = static_cast<uint32_t>(sizeof(FSAResponse));

   auto &request = shim->request.writeFile;
   request.buffer = virt_cast<uint8_t *>(buffer);
   request.size = size;
   request.count = count;
   request.pos = pos;
   request.handle = handle;
   request.writeFlags = writeFlags;

   return FSAStatus::OK;
}

} // namespace internal

void
Library::registerFsaShimSymbols()
{
   RegisterFunctionExportName("__FSAShimDecodeIosErrorToFsaStatus", FSAShimDecodeIosErrorToFsaStatus);
}

} // namespace cafe::coreinit
