#include "cafe_loader_bounce.h"
#include "cafe_loader_error.h"
#include "cafe_loader_iop.h"
#include "cafe_loader_loaded_rpl.h"
#include "cafe_loader_log.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/kernel/cafe_kernel_mmu.h"
#include "cafe/kernel/cafe_kernel_processid.h"
#include "ios/ios_enum.h"
#include "ios/fs/ios_fs_enum.h"
#include "ios/mcp/ios_mcp_enum.h"
#include "ios/mcp/ios_mcp_mcp_request.h"
#include "kernel/kernel_filesystem.h"

#include <libcpu/be2_struct.h>

using namespace ios::mcp;
using namespace cafe::kernel;

namespace cafe::loader::internal
{

constexpr auto ChunkSize = 0x400000u;
static bool sgFinishedLoadingBuffer = false;
static MCPFileType sgFileType = MCPFileType::ProcessCode;
static UniqueProcessId sgProcId = UniqueProcessId::Invalid;
static uint32_t sgGotBytes = 0;
static uint32_t sgTotalBytes = 0;
static uint32_t sgFileOffset = 0;
static uint32_t sgBufferNumber = 0;
static ios::Error sgBounceError = ios::Error::OK;
static std::string sgLoadName;

void
LiInitBuffer(bool unk)
{
   if (unk) {
      sgFinishedLoadingBuffer = true;
   }

   sgFileType = MCPFileType::ProcessCode;
   sgFileOffset = 0u;
   sgBufferNumber = 0u;
   sgProcId = UniqueProcessId::Invalid;
   sgLoadName.clear();
   sgTotalBytes = 0u;
   sgBounceError = ios::Error::OK;
   sgGotBytes = 0u;
}

ios::Error
LiBounceOneChunk(std::string_view name,
                 ios::mcp::MCPFileType fileType,
                 kernel::UniqueProcessId upid,
                 uint32_t *outChunkBytes,
                 uint32_t offset,
                 uint32_t bufferNumber,
                 virt_ptr<void> *outChunk)
{
   LiCheckAndHandleInterrupts();
   auto bounceBufferMapping =
      kernel::getVirtualMemoryMap(kernel::VirtualMemoryRegion::LoaderBounceBuffer);
   auto bounceBufferAddr = bounceBufferMapping.vaddr;
   if (bufferNumber != 1) {
      bounceBufferAddr += ChunkSize;
   }

   sgLoadName = name;
   sgFileOffset = offset;
   sgBufferNumber = bufferNumber;
   sgFileType = fileType;
   sgProcId = upid;

   auto error = LiLoadAsync(name,
                            virt_cast<void *>(bounceBufferAddr),
                            ChunkSize,
                            offset,
                            fileType,
                            getRamPartitionIdFromUniqueProcessId(upid));
   sgBounceError = error;
   if (error < ios::Error::OK) {
      LiSetFatalError(0x1872A7, fileType, 0, "LiBounceOneChunk", 131);
      Loader_ReportError("***LiLoadAsync failed. err={}.", error);
      LiCheckAndHandleInterrupts();
      return error;
   }

   if (outChunkBytes) {
      *outChunkBytes = ChunkSize;
   }

   if (outChunk) {
      *outChunk = virt_cast<void *>(bounceBufferAddr);
   }

   sgFinishedLoadingBuffer = (sgFinishedLoadingBuffer == 0) ? TRUE : FALSE;
   LiCheckAndHandleInterrupts();
   return ios::Error::OK;
}

ios::Error
LiWaitOneChunk(uint32_t *outBytesRead,
               std::string_view name,
               MCPFileType fileType)
{
   auto bytesRead = uint32_t { 0 };
   auto error = LiWaitIopCompleteWithInterrupts(&bytesRead);
   sgBounceError = error;
   if (error < ios::Error::OK) {
      return error;
   }

   sgGotBytes = bytesRead;
   sgTotalBytes += bytesRead;

   if (outBytesRead) {
      *outBytesRead = bytesRead;
   }

   sgFinishedLoadingBuffer = (sgFinishedLoadingBuffer == 0) ? TRUE : FALSE;
   return ios::Error::OK;
}

int32_t
LiCleanUpBufferAfterModuleLoaded()
{
   if (!sgLoadName[0] || sgFinishedLoadingBuffer) {
      return 0;
   }

   if (sgTotalBytes & (ChunkSize - 1)) {
      Loader_ReportError(
         "LiCleanUpBufferAfterModuleLoaded: {} finished loading but left load buffers in unusable state; error {} (0x{:08X}).",
         sgLoadName,
         -470105, -470105);
      LiSetFatalError(0x18729Bu, sgFileType, 1, "LiCleanUpBufferAfterModuleLoaded", 359);
      return -470105;
   }

   auto bytesRead = uint32_t { 0 };
   auto error = LiWaitOneChunk(&bytesRead,
                               sgLoadName,
                               sgFileType);
   if (error) {
      Loader_ReportError(
         "LiCleanUpBufferAfterModuleLoaded: Loader incorrectly calculated that {} finished loading but got error {} (0x{:08X}).",
         sgLoadName, error, error);
      return error;
   }

   if (bytesRead) {
      Loader_ReportError(
         "LiCleanUpBufferAfterModuleLoaded: Loader incorrectly calculated that {} finished loading; error {} (0x{:08X}).",
         sgLoadName, -470105, -470105);
      LiSetFatalError(0x18729Bu, sgFileType, 1, "LiCleanUpBufferAfterModuleLoaded", 367);
      return -470105;
   }

   return 0;
}

int32_t
LiRefillUpcomingBounceBuffer(virt_ptr<LOADED_RPL> rpl,
                             int32_t bufferNumber)
{
   auto chunkReadSize = uint32_t { 0 };
   auto chunkBuffer = virt_ptr<void> { nullptr };
   auto error =
      LiBounceOneChunk(virt_cast<char *>(rpl->pathBuffer).getRawPointer(),
                       rpl->fileType,
                       rpl->upid,
                       &chunkReadSize,
                       rpl->upcomingFileOffset,
                       bufferNumber,
                       &chunkBuffer);
   rpl->chunkBuffer = chunkBuffer;

   if (error != ios::Error::OK) {
      Loader_ReportError(
         "***LiBounceOneChunk failed loading \"{}\" of type {} at offset 0x{:08X} err={}.",
         rpl->pathBuffer,
         rpl->fileType,
         rpl->upcomingFileOffset,
         error);
   }

   return error;
}

void
LiCloseBufferIfError()
{
   if (sgLoadName[0]) {
      auto loadName = std::string_view { sgLoadName };

      if (!sgFinishedLoadingBuffer) {
         LiWaitOneChunk(NULL, loadName, sgFileType);
      }

      if (sgBounceError == ios::Error::OK) {
         while(sgGotBytes == ChunkSize) {
            Loader_ReportWarn("***Loader completely reading {} from offset 0x{:08X}.",
                              loadName, sgFileOffset + ChunkSize);

            LiBounceOneChunk(loadName,
                             sgFileType,
                             sgProcId,
                             nullptr,
                             sgFileOffset + sgGotBytes,
                             (sgBufferNumber == 1) ? 2 : 1,
                             nullptr);
            LiWaitOneChunk(NULL, loadName, sgFileType);
         }
      }
   }

   LiInitBuffer(false);
}

} // namespace cafe::loader::internal
