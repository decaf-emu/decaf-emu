#include "coreinit.h"
#include "coreinit_fsa.h"
#include "coreinit_fsa_shim.h"
#include "coreinit_ios.h"

#include "cafe/cafe_stackobject.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>

namespace cafe::coreinit
{

namespace internal
{

static FSAStatus
fsaGetInfoByQuery(FSAClientHandle clientHandle,
                  virt_ptr<const char> path,
                  FSAQueryInfoType type,
                  virt_ptr<void> out);

} // namespace internal

FSAStatus
FSACloseFile(FSAClientHandle clientHandle,
             FSAFileHandle fileHandle)
{
   auto shimBuffer = StackObject<virt_ptr<FSAShimBuffer>> { };

   if (!FSAShimCheckClientHandle(clientHandle)) {
      return FSAStatus::InvalidClientHandle;
   }

   auto status = FSAShimAllocateBuffer(shimBuffer);
   if (status < FSAStatus::OK) {
      return status;
   }

   status = internal::fsaShimPrepareRequestCloseFile(*shimBuffer,
                                                     clientHandle,
                                                     fileHandle);
   if (status >= FSAStatus::OK) {
      status = internal::fsaShimSubmitRequest(*shimBuffer, FSAStatus::OK);
   }

   FSAShimFreeBuffer(*shimBuffer);
   return status;
}

FSAStatus
FSAGetStat(FSAClientHandle clientHandle,
           virt_ptr<const char> path,
           virt_ptr<FSAStat> stat)
{
   return internal::fsaGetInfoByQuery(clientHandle,
                                      path,
                                      FSAQueryInfoType::Stat,
                                      stat);
}

FSAStatus
FSAMakeDir(FSAClientHandle clientHandle,
           virt_ptr<const char> path,
           uint32_t permissions)
{
   auto shimBuffer = StackObject<virt_ptr<FSAShimBuffer>> { };

   if (!FSAShimCheckClientHandle(clientHandle)) {
      return FSAStatus::InvalidClientHandle;
   }

   auto status = FSAShimAllocateBuffer(shimBuffer);
   if (status < FSAStatus::OK) {
      return status;
   }

   status = internal::fsaShimPrepareRequestMakeDir(*shimBuffer,
                                                   clientHandle,
                                                   path,
                                                   permissions);
   if (status >= FSAStatus::OK) {
      status = internal::fsaShimSubmitRequest(*shimBuffer, FSAStatus::OK);
   }

   FSAShimFreeBuffer(*shimBuffer);
   return status;
}

FSAStatus
FSAMount(FSAClientHandle clientHandle,
         virt_ptr<const char> path,
         virt_ptr<const char> target,
         uint32_t unk0,
         virt_ptr<void> unkBuf,
         uint32_t unkBufLen)
{
   auto shimBuffer = StackObject<virt_ptr<FSAShimBuffer>> { };

   if (!FSAShimCheckClientHandle(clientHandle)) {
      return FSAStatus::InvalidClientHandle;
   }

   auto status = FSAShimAllocateBuffer(shimBuffer);
   if (status < FSAStatus::OK) {
      return status;
   }

   status = internal::fsaShimPrepareRequestMount(*shimBuffer,
                                                 clientHandle,
                                                 path,
                                                 target,
                                                 unk0,
                                                 unkBuf,
                                                 unkBufLen);
   if (status >= FSAStatus::OK) {
      status = internal::fsaShimSubmitRequest(*shimBuffer, FSAStatus::OK);
   }

   FSAShimFreeBuffer(*shimBuffer);
   return status;
}

FSAStatus
FSAOpenFile(FSAClientHandle clientHandle,
            virt_ptr<const char> path,
            virt_ptr<const char> mode,
            virt_ptr<FSAFileHandle> outHandle)
{
   auto shimBuffer = StackObject<virt_ptr<FSAShimBuffer>> { };

   if (!FSAShimCheckClientHandle(clientHandle)) {
      return FSAStatus::InvalidClientHandle;
   }

   auto status = FSAShimAllocateBuffer(shimBuffer);
   if (status < FSAStatus::OK) {
      return status;
   }

   status = internal::fsaShimPrepareRequestOpenFile(*shimBuffer,
                                                    clientHandle,
                                                    path,
                                                    mode,
                                                    0x660,
                                                    0,
                                                    0);
   if (status >= FSAStatus::OK) {
      status = internal::fsaShimSubmitRequest(*shimBuffer, FSAStatus::OK);
   }

   if (status >= FSAStatus::OK) {
      *outHandle = (*shimBuffer)->response.openFile.handle;
   }

   FSAShimFreeBuffer(*shimBuffer);
   return status;
}

FSAStatus
FSAReadFile(FSAClientHandle clientHandle,
             virt_ptr<uint8_t> buffer,
             uint32_t size,
             uint32_t count,
             FSAFileHandle fileHandle,
             FSAReadFlag readFlags)
{
   auto shimBuffer = StackObject<virt_ptr<FSAShimBuffer>> { };

   if (!FSAShimCheckClientHandle(clientHandle)) {
      return FSAStatus::InvalidClientHandle;
   }

   auto status = FSAShimAllocateBuffer(shimBuffer);
   if (status < FSAStatus::OK) {
      return status;
   }

   status =
      internal::fsaShimPrepareRequestReadFile(*shimBuffer,
                                              clientHandle,
                                              buffer,
                                              size,
                                              count,
                                              0,
                                              fileHandle,
                                              readFlags & ~FSAReadFlag::ReadWithPos);
   if (status >= FSAStatus::OK) {
      status = internal::fsaShimSubmitRequest(*shimBuffer, FSAStatus::OK);
   }

   FSAShimFreeBuffer(*shimBuffer);
   return status;
}

FSAStatus
FSARemove(FSAClientHandle clientHandle,
          virt_ptr<const char> path)
{
   auto shimBuffer = StackObject<virt_ptr<FSAShimBuffer>> { };

   if (!FSAShimCheckClientHandle(clientHandle)) {
      return FSAStatus::InvalidClientHandle;
   }

   auto status = FSAShimAllocateBuffer(shimBuffer);
   if (status < FSAStatus::OK) {
      return status;
   }

   status = internal::fsaShimPrepareRequestRemove(*shimBuffer,
                                                  clientHandle,
                                                  path);
   if (status >= FSAStatus::OK) {
      status = internal::fsaShimSubmitRequest(*shimBuffer, FSAStatus::OK);
   }

   FSAShimFreeBuffer(*shimBuffer);
   return status;
}

FSAStatus
FSAWriteFile(FSAClientHandle clientHandle,
             virt_ptr<const uint8_t> buffer,
             uint32_t size,
             uint32_t count,
             FSAFileHandle fileHandle,
             FSAWriteFlag writeFlags)
{
   auto shimBuffer = StackObject<virt_ptr<FSAShimBuffer>> { };

   if (!FSAShimCheckClientHandle(clientHandle)) {
      return FSAStatus::InvalidClientHandle;
   }

   auto status = FSAShimAllocateBuffer(shimBuffer);
   if (status < FSAStatus::OK) {
      return status;
   }

   status =
      internal::fsaShimPrepareRequestWriteFile(*shimBuffer,
                                               clientHandle,
                                               buffer,
                                               size,
                                               count,
                                               0,
                                               fileHandle,
                                               writeFlags & ~FSAWriteFlag::WriteWithPos);
   if (status >= FSAStatus::OK) {
      status = internal::fsaShimSubmitRequest(*shimBuffer, FSAStatus::OK);
   }

   FSAShimFreeBuffer(*shimBuffer);
   return status;
}

namespace internal
{

FSAStatus
fsaGetInfoByQuery(FSAClientHandle clientHandle,
                  virt_ptr<const char> path,
                  FSAQueryInfoType type,
                  virt_ptr<void> out)
{
   auto shimBuffer = StackObject<virt_ptr<FSAShimBuffer>> { };

   if (!FSAShimCheckClientHandle(clientHandle)) {
      return FSAStatus::InvalidClientHandle;
   }

   auto status = FSAShimAllocateBuffer(shimBuffer);
   if (status < FSAStatus::OK) {
      return status;
   }

   status = internal::fsaShimPrepareRequestGetInfoByQuery(*shimBuffer,
                                                          clientHandle,
                                                          path,
                                                          type);
   if (status >= FSAStatus::OK) {
      status = internal::fsaShimSubmitRequest(*shimBuffer, FSAStatus::OK);
   }

   if (status >= FSAStatus::OK) {
      switch (type) {
      case FSAQueryInfoType::FreeSpaceSize:
      {
         auto freeSpaceSize = virt_cast<uint64_t *>(out);
         *freeSpaceSize = (*shimBuffer)->response.getInfoByQuery.freeSpaceSize;
         break;
      }
      case FSAQueryInfoType::DirSize:
      {
         auto dirSize = virt_cast<uint64_t *>(out);
         *dirSize = (*shimBuffer)->response.getInfoByQuery.dirSize;
         break;
      }
      case FSAQueryInfoType::EntryNum:
      {
         auto entryNum = virt_cast<int32_t *>(out);
         *entryNum = (*shimBuffer)->response.getInfoByQuery.entryNum;
         break;
      }
      case FSAQueryInfoType::FileSystemInfo:
      {
         auto fileSystemInfo = virt_cast<FSAFileSystemInfo *>(out);
         *fileSystemInfo = (*shimBuffer)->response.getInfoByQuery.fileSystemInfo;
         break;
      }
      case FSAQueryInfoType::DeviceInfo:
      {
         auto deviceInfo = virt_cast<FSADeviceInfo *>(out);
         *deviceInfo = (*shimBuffer)->response.getInfoByQuery.deviceInfo;
         break;
      }
      case FSAQueryInfoType::Stat:
      {
         auto stat = virt_cast<FSAStat *>(out);
         *stat = (*shimBuffer)->response.getInfoByQuery.stat;
         break;
      }
      case FSAQueryInfoType::BadBlockInfo:
      {
         auto badBlockInfo = virt_cast<FSABlockInfo *>(out);
         *badBlockInfo = (*shimBuffer)->response.getInfoByQuery.badBlockInfo;
         break;
      }
      case FSAQueryInfoType::JournalFreeSpaceSize:
      {
         auto freeSpaceSize = virt_cast<uint64_t *>(out);
         *freeSpaceSize = (*shimBuffer)->response.getInfoByQuery.journalFreeSpaceSize;
         break;
      }
      default:
         decaf_abort(fmt::format("Unexpected QueryInfoType: {}", type));
      }
   }

   FSAShimFreeBuffer(*shimBuffer);
   return status;
}

} // namespace internal

void
Library::registerFsaCmdSymbols()
{
   RegisterFunctionExport(FSACloseFile);
   RegisterFunctionExport(FSAGetStat);
   RegisterFunctionExport(FSAMakeDir);
   RegisterFunctionExport(FSAMount);
   RegisterFunctionExport(FSAOpenFile);
   RegisterFunctionExport(FSAReadFile);
   RegisterFunctionExport(FSARemove);
   RegisterFunctionExport(FSAWriteFile);
}

} // namespace cafe::coreinit
