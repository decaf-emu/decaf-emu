#include "cafe_loader_bounce.h"
#include "cafe_loader_error.h"
#include "cafe_loader_iop.h"
#include "cafe_loader_loaded_rpl.h"
#include "cafe_loader_log.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/cafe_hle.h"
#include "cafe/kernel/cafe_kernel_mmu.h"
#include "cafe/kernel/cafe_kernel_processid.h"
#include "ios/ios_enum.h"
#include "ios/fs/ios_fs_enum.h"
#include "ios/mcp/ios_mcp_enum.h"
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
LiLoadAsync(std::string_view name,
            virt_ptr<void> outBuffer,
            uint32_t outBufferSize,
            uint32_t pos,
            MCPFileType fileType,
            RamPartitionId rampid)
{
   auto path = std::string { };

   if (fileType == MCPFileType::CafeOS) {
      auto library = cafe::hle::getLibrary(name);
      if (library) {
         auto &rpl = library->getGeneratedRpl();
         auto bytesRead = std::min<uint32_t>(rpl.size() - pos, outBufferSize);
         std::memcpy(outBuffer.getRawPointer(), rpl.data() + pos, bytesRead);
         sgBounceError = ios::Error::OK;
         sgGotBytes = bytesRead;
         sgTotalBytes += bytesRead;
         return ios::Error::OK;
      }
   }

   switch (fileType) {
   case MCPFileType::ProcessCode:
      path = fmt::format("/vol/code/{}", name);
      break;
   case MCPFileType::CafeOS:
      path = fmt::format("/vol/storage_mlc01/sys/title/00050010/1000400A/code/{}", name);
      break;
   default:
      sgBounceError = ios::Error::NoExists;
      return ios::Error::NoExists;
   }

   auto result = ::kernel::getFileSystem()->openFile(path, fs::File::Read);
   if (!result) {
      sgBounceError = ios::Error::NoExists;
      return ios::Error::NoExists;
   }

   auto file = result.value();
   file->seek(pos);
   auto bytesRead = static_cast<uint32_t>(file->read(outBuffer.getRawPointer(), 1, outBufferSize));
   sgBounceError = ios::Error::OK;
   sgGotBytes = bytesRead;
   sgTotalBytes += bytesRead;
   return ios::Error::OK;

   /* TODO: When we move to IPC...
   auto request = virt_cast<MCPRequestLoadFile *>(iop_percore_malloc(sizeof(MCPRequestLoadFile)));
   request->pos = pos;
   request->fileType = fileType;
   request->cafeProcessId = static_cast<uint32_t>(rampid);
   request->name = name;

   sIopData->loadReply.done = FALSE;
   sIopData->loadReply.pending = TRUE;
   sIopData->loadReply.requestBuffer = request;
   sIopData->loadReply.error = ios::Error::InvalidArg;

   auto error = IPCLDriver_IoctlAsync(sIopData->mcpHandle,
                                      MCPCommand::LoadFile,
                                      request, sizeof(MCPRequestLoadFile),
                                      outBuffer, outBufferSize,
                                      &Loader_AsyncCallback,
                                      virt_addrof(sIopData->loadReply));
   if (error < ios::Error::OK) {
      iop_percore_free(request);
   }

   return error;
   */
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

   if (outChunkBytes) {
      *outChunkBytes = ChunkSize;
   }

   if (outChunk) {
      *outChunk = virt_cast<void *>(bounceBufferAddr);
   }

   sgFinishedLoadingBuffer = (sgFinishedLoadingBuffer == 0) ? TRUE : FALSE;
   return ios::Error::OK;
}

ios::Error
LiWaitOneChunk(uint32_t *outBytesRead,
               std::string_view name,
               MCPFileType fileType)
{
   sgFinishedLoadingBuffer = (sgFinishedLoadingBuffer == 0) ? TRUE : FALSE;

   if (outBytesRead) {
      *outBytesRead = sgGotBytes;
   }

   return sgBounceError;
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
