#include "cafe_loader_basics.h"
#include "cafe_loader_bounce.h"
#include "cafe_loader_elffile.h"
#include "cafe_loader_error.h"
#include "cafe_loader_globals.h"
#include "cafe_loader_heap.h"
#include "cafe_loader_iop.h"
#include "cafe_loader_log.h"
#include "cafe_loader_loaded_rpl.h"
#include "cafe_loader_zlib.h"

#include "cafe/cafe_tinyheap.h"
#include "cafe/cafe_stackobject.h"
#include <libcpu/be2_struct.h>

namespace cafe::loader::internal
{

constexpr auto MaxSDKVersion = 21301u;
constexpr auto MinSDKVersion = 20500u;

static virt_ptr<rpl::SectionHeader>
getSectionHeader(virt_ptr<LOADED_RPL> rpl,
                 virt_ptr<rpl::SectionHeader> sectionHeaderBuffer,
                 uint32_t idx)
{
   auto base = virt_cast<virt_addr>(sectionHeaderBuffer);
   auto offset = idx * rpl->elfHeader.shentsize;
   return virt_cast<rpl::SectionHeader *>(base + offset);
}

static virt_ptr<rpl::SectionHeader>
getSectionHeader(virt_ptr<LOADED_RPL> rpl,
                 uint32_t idx)
{
   return getSectionHeader(rpl,
                           virt_cast<rpl::SectionHeader *>(rpl->sectionHeaderBuffer),
                           idx);
}

static int32_t
LiInitBufferTracking(LiBasicsLoadArgs *loadArgs)
{
   virt_ptr<void> allocPtr;
   uint32_t allocSize;
   uint32_t largestFree;
   auto error = LiCacheLineCorrectAllocEx(loadArgs->readHeapTracking,
                                          align_up(loadArgs->pathNameLen + 1, 4),
                                          4,
                                          &allocPtr,
                                          1,
                                          &allocSize,
                                          &largestFree,
                                          loadArgs->fileType);
   if (error != 0) {
      return error;
   }

   auto rpl = loadArgs->loadedRpl;
   rpl->pathBuffer = allocPtr;
   rpl->pathBufferSize = allocSize;
   std::strncpy(virt_cast<char *>(rpl->pathBuffer).getRawPointer(),
                loadArgs->pathName.getRawPointer(),
                loadArgs->pathNameLen + 1);

   rpl->upcomingBufferNumber = 1u;
   rpl->lastChunkBuffer = loadArgs->chunkBuffer;
   rpl->fileOffset = loadArgs->fileOffset;
   rpl->upcomingFileOffset = loadArgs->chunkBufferSize;
   rpl->totalBytesRead = loadArgs->chunkBufferSize;
   rpl->upid = loadArgs->upid;
   rpl->fileType = loadArgs->fileType;
   rpl->virtualFileBaseOffset = loadArgs->chunkBufferSize;

   if (loadArgs->chunkBufferSize == 0x400000) {
      error = LiRefillUpcomingBounceBuffer(rpl, 2);
   } else {
      LiInitBuffer(false);
   }

   if (error != 0 && rpl->pathBuffer) {
      LiCacheLineCorrectFreeEx(loadArgs->readHeapTracking, rpl->pathBuffer, rpl->pathBufferSize);
   }

   return error;
}

static int32_t
LiCheckFileBounds(virt_ptr<LOADED_RPL> rpl)
{
   auto shBase = virt_cast<virt_addr>(rpl->sectionHeaderBuffer);
   auto dataMin = -1;
   auto dataMax = 0;

   auto readMin = -1;
   auto readMax = 0;

   auto textMin = -1;
   auto textMax = 0;

   auto tempMin = -1;
   auto tempMax = 0;

   for (auto i = 0u; i < rpl->elfHeader.shnum; ++i) {
      auto sectionHeader = virt_cast<rpl::SectionHeader *>(shBase + i * rpl->elfHeader.shentsize);
      if (sectionHeader->size == 0 ||
          sectionHeader->type == rpl::SHT_RPL_FILEINFO ||
          sectionHeader->type == rpl::SHT_RPL_CRCS ||
          sectionHeader->type == rpl::SHT_NOBITS ||
          sectionHeader->type == rpl::SHT_RPL_IMPORTS) {
         continue;
      }

      if ((sectionHeader->flags & rpl::SHF_EXECINSTR) &&
          sectionHeader->type != rpl::SHT_RPL_EXPORTS) {
         textMin = std::min(textMin, static_cast<int32_t>(sectionHeader->offset));
         textMax = std::min(textMax, static_cast<int32_t>(sectionHeader->offset + sectionHeader->size));
      } else {
         if (sectionHeader->flags & rpl::SHF_ALLOC) {
            if (sectionHeader->flags & rpl::SHF_WRITE) {
               dataMin = std::min(dataMin, static_cast<int32_t>(sectionHeader->offset));
               dataMax = std::min(dataMax, static_cast<int32_t>(sectionHeader->offset + sectionHeader->size));
            } else {
               readMin = std::min(readMin, static_cast<int32_t>(sectionHeader->offset));
               readMax = std::min(readMax, static_cast<int32_t>(sectionHeader->offset + sectionHeader->size));
            }
         } else {
            tempMin = std::min(tempMin, static_cast<int32_t>(sectionHeader->offset));
            tempMax = std::min(tempMax, static_cast<int32_t>(sectionHeader->offset + sectionHeader->size));
         }
      }
   }

   if (dataMin == -1) {
      dataMin = (rpl->elfHeader.shnum * rpl->elfHeader.shentsize) + rpl->elfHeader.shoff;
      dataMax = dataMin;
   }

   if (readMin == -1) {
      readMin = dataMax;
      readMax = dataMax;
   }

   if (textMin == -1) {
      textMin = readMax;
      textMax = readMax;
   }

   if (tempMin == -1) {
      tempMin = textMax;
      tempMax = textMax;
   }

   if (static_cast<uint32_t>(dataMin) < rpl->elfHeader.shoff) {
      Loader_ReportError("*** SecHrs, FileInfo, or CRCs in bad spot in file. Return %d.",
                         Error::CheckFileBoundsFailed);
      goto error;
   }

   // Data
   if (dataMin > dataMax) {
      Loader_ReportError("*** DataMin > DataMax. break.");
      goto error;
   }

   if (dataMin > readMin) {
      Loader_ReportError("*** DataMin > ReadMin. break.");
      goto error;
   }

   if (dataMax > readMin) {
      Loader_ReportError("*** DataMax > ReadMin. break.");
      goto error;
   }

   // Read
   if (readMin > readMax) {
      Loader_ReportError("*** ReadMin > ReadMax. break.");
      goto error;
   }

   if (readMin > textMin) {
      Loader_ReportError("*** ReadMin > TextMin. break.");
      goto error;
   }

   if (readMax > textMin) {
      Loader_ReportError("*** ReadMax > TextMin. break.");
      goto error;
   }

   // Text
   if (textMin > textMax) {
      Loader_ReportError("*** TextMin > TextMax. break.");
      goto error;
   }

   if (textMin > tempMin) {
      Loader_ReportError("*** TextMin > TempMin. break.");
      goto error;
   }

   if (textMax > tempMin) {
      Loader_ReportError("*** TextMax > TempMin. break.");
      goto error;
   }

   // Temp
   if (tempMin > tempMax) {
      Loader_ReportError("*** TempMin > TempMax. break.");
      goto error;
   }

   return 0;

error:
   LiSetFatalError(0x18729B, rpl->fileType, 1, "LiCheckFileBounds", 0x247);
   return Error::CheckFileBoundsFailed;
}

int32_t
LiLoadRPLBasics(virt_ptr<char> moduleName,
                uint32_t moduleNameLen,
                virt_ptr<void> chunkBuffer,
                virt_ptr<TinyHeap> codeHeapTracking,
                virt_ptr<TinyHeap> dataHeapTracking,
                bool allocModuleName,
                uint32_t r9,
                virt_ptr<LOADED_RPL> *outLoadedRpl,
                LiBasicsLoadArgs *loadArgs,
                uint32_t arg_C)
{
   struct LoadAttemptErrorData
   {
      int32_t error;
      ios::mcp::MCPFileType fileType;
      uint32_t fatalErr;
      std::string fatalFunction;
      uint32_t fatalLine;
      uint32_t fatalMsgType;
   };

   std::array<LoadAttemptErrorData, 3> loadAttemptErrors;
   auto globals = getGlobalStorage();
   auto loadAttempt = int32_t { 0 };
   auto chunkReadSize = uint32_t { 0 };
   auto error = int32_t { 0 };

   while (true) {
      error = LiWaitOneChunk(&chunkReadSize, loadArgs->pathName.getRawPointer(), loadArgs->fileType);
      if (error == 0) {
         break;
      }

      if (loadAttempt < 2) {
         if (LiGetFatalError()) {
            auto &attemptErrors = loadAttemptErrors[loadAttempt];
            attemptErrors.error = error;
            attemptErrors.fileType = loadArgs->fileType;
            attemptErrors.fatalErr = LiGetFatalError();
            attemptErrors.fatalFunction = LiGetFatalFunction();
            attemptErrors.fatalLine = LiGetFatalLine();
            attemptErrors.fatalMsgType = LiGetFatalMsgType();
            LiResetFatalError();
         }

         if (loadAttempt == 0) {
            if (loadArgs->fileType == ios::mcp::MCPFileType::ProcessCode) {
               loadArgs->fileType = ios::mcp::MCPFileType::CafeOS;
            }
         } else {
            loadArgs->fileType = ios::mcp::MCPFileType::SystemDataCode;
         }

         LiInitBuffer(false);
         chunkReadSize = 0;
         loadArgs->chunkBufferSize = 0u;

         auto outChunkBufferSize = uint32_t { 0 };
         auto outChunkBuffer = virt_ptr<void> { nullptr };
         error = LiBounceOneChunk(loadArgs->pathName.getRawPointer(),
                                  loadArgs->fileType,
                                  loadArgs->upid,
                                  &outChunkBufferSize,
                                  loadArgs->fileOffset,
                                  1,
                                  &outChunkBuffer);
         loadArgs->chunkBuffer = outChunkBuffer;
         loadArgs->chunkBufferSize = outChunkBufferSize;
         ++loadAttempt;
      }

      if (error != 0) {
         Loader_ReportError("***Loader failure {} first time, {} second time and {} third time. loading \"{}\".",
                            loadAttemptErrors[0].error,
                            loadAttemptErrors[1].error,
                            error,
                            loadArgs->pathName.getRawPointer());
         return error;
      }
   }

   loadArgs->chunkBufferSize = chunkReadSize;
   LiCheckAndHandleInterrupts();

   // Load and validate the ELF header
   auto filePhStride = uint32_t { 0 };
   auto fileShStride = uint32_t { 0 };
   auto fileElfHeader = virt_ptr<rpl::Header> { nullptr };
   auto fileSectionHeaders = virt_ptr<rpl::SectionHeader> { nullptr };
   error = ELFFILE_ValidateAndPrepareMinELF(chunkBuffer,
                                            chunkReadSize,
                                            &fileElfHeader,
                                            &fileSectionHeaders,
                                            &fileShStride,
                                            &filePhStride);
   if (error) {
      Loader_ReportError("*** Failed ELF file checks (err=0x{:08X}", error);
      LiSetFatalError(0x18729B, loadArgs->fileType, 1, "LiLoadRPLBasics", 0x325);
      return error;
   }

   // Check that this ELF looks like a Wii U Cafe RPL
   if (fileElfHeader->fileClass != rpl::ELFCLASS32 ||
       fileElfHeader->encoding != rpl::ELFDATA2MSB ||
       fileElfHeader->abi != rpl::EABI_CAFE ||
       fileElfHeader->elfVersion > rpl::EV_CURRENT ||
       fileElfHeader->machine != rpl::EM_PPC ||
       fileElfHeader->version != 1 ||
       fileElfHeader->shnum < 2) {
      return -470025;
   }

   // Initialise temporary RPL basics
   cafe::StackObject<LOADED_RPL> tmpLoadedRpl;
   virt_ptr<LOADED_RPL> rpl;
   rpl = tmpLoadedRpl;

   std::memset(rpl.getRawPointer(), 0, sizeof(LOADED_RPL));
   if (r9 == 0) {
      rpl->globals = getGlobalStorage();
   }

   std::memcpy(virt_addrof(rpl->elfHeader).getRawPointer(),
               fileElfHeader.getRawPointer(),
               sizeof(rpl::Header));

   // Check some offsets are valid
   if (!rpl->elfHeader.shentsize) {
      rpl->elfHeader.shentsize = static_cast<uint16_t>(sizeof(rpl::SectionHeader));
   }

   if (rpl->elfHeader.shoff >= chunkReadSize ||
       ((rpl->elfHeader.shnum - 1) * rpl->elfHeader.shentsize) + rpl->elfHeader.shoff >= chunkReadSize) {
      LiSetFatalError(0x18729B, loadArgs->fileType, 1, "LiLoadRPLBasics", 0x33F);
      return -470077;
   }

   auto shRplCrcs = getSectionHeader(rpl, fileSectionHeaders, rpl->elfHeader.shnum - 2);
   if (shRplCrcs->offset >= chunkReadSize ||
       shRplCrcs->offset + shRplCrcs->size >= chunkReadSize) {
      LiSetFatalError(0x18729B, loadArgs->fileType, 1, "LiLoadRPLBasics", 0x348);
      return -470077;
   }

   auto shRplFileInfo = getSectionHeader(rpl, fileSectionHeaders, rpl->elfHeader.shnum - 1);
   if (shRplFileInfo->offset + shRplFileInfo->size >= chunkReadSize) {
      LiSetFatalError(0x18729B, loadArgs->fileType, 1, "LiLoadRPLBasics", 0x351);
      return -470078;
   }

   // Check RPL file info
   if (shRplFileInfo->type != rpl::SHT_RPL_FILEINFO ||
       shRplFileInfo->flags & rpl::SHF_DEFLATED) {
      Loader_ReportError("***shnum-1 section type = 0x{:08X}, flags=0x{:08X}", shRplFileInfo->type, shRplFileInfo->flags);
      LiSetFatalError(0x18729B, loadArgs->fileType, 1, "LiLoadRPLBasics", 0x35A);
      return -470082;
   }

   auto fileInfo = virt_cast<rpl::RPLFileInfo_v4_2 *>(virt_cast<virt_addr>(fileElfHeader) + shRplFileInfo->offset);
   if (fileInfo->version < rpl::RPLFileInfo_v4_2::Version) {
      Loader_ReportError("*** COS requires that {} be built with at least SDK {}.{:02}.{}, it was built with an older SDK",
                         moduleName,
                         2, 5, 0);
      LiSetFatalError(0x187298, loadArgs->fileType, 1, "LiLoadRPLBasics", 0x38B);
      return -470062;
   }

   if (fileInfo->minVersion < MinSDKVersion || fileInfo->minVersion > MaxSDKVersion) {
      auto major = fileInfo->minVersion / 10000;
      auto minor = (fileInfo->minVersion % 10000) / 100;
      auto patch = fileInfo->minVersion % 100;
      Loader_ReportError("*** COS requires that {} be built with at least SDK {}.{:02}.{}, it was built with SDK {}.{:02}.{}",
                         moduleName,
                         2, 5, 0,
                         major, minor, patch);
      LiSetFatalError(0x187298, loadArgs->fileType, 1, "LiLoadRPLBasics", 0x38B);
      return -470062;
   }

   auto allocPtr = virt_ptr<void> { nullptr };
   auto allocSize = uint32_t { 0 };
   auto largestFree = uint32_t { 0 };
   auto sectionCrcs = virt_ptr<uint32_t> { nullptr };
   auto fileInfoCrc = uint32_t { 0 };

   if (fileInfo->textSize) {
      error = LiCacheLineCorrectAllocEx(codeHeapTracking,
                                        fileInfo->textSize,
                                        fileInfo->textAlign,
                                        &allocPtr,
                                        0,
                                        &allocSize,
                                        &largestFree,
                                        loadArgs->fileType);
      if (error != 0) {
         Loader_ReportError("***Could not allocate uncompressed text ({}) in {} heap \"{}\";  (needed {}, available {}).",
                            fileInfo->textSize,
                            r9 ? "shared code" : "code",
                            moduleName,
                            fileInfo->textSize,
                            largestFree);
         goto lblError;
      }

      rpl->textBuffer = allocPtr;
      rpl->textBufferSize = allocSize;
   }

   error = LiCacheLineCorrectAllocEx(dataHeapTracking,
                                     sizeof(LOADED_RPL),
                                     4,
                                     &allocPtr,
                                     0,
                                     &allocSize,
                                     &largestFree,
                                     loadArgs->fileType);
   if (error != 0) {
      Loader_ReportError("*** Failed {} alloc (err=0x{:08X});  (needed {}, available {})",
                         r9 ? "readheap" : "workarea",
                         error,
                         sizeof(LOADED_RPL),
                         largestFree);
      goto lblError;
   }

   rpl = virt_cast<LOADED_RPL *>(allocPtr);
   std::memcpy(rpl.getRawPointer(),
               tmpLoadedRpl.getRawPointer(),
               sizeof(LOADED_RPL));

   rpl->selfBufferSize = allocSize;
   loadArgs->loadedRpl = rpl;

   error = LiInitBufferTracking(loadArgs);
   if (error != 0) {
      goto lblError;
   }

   error = LiCacheLineCorrectAllocEx(dataHeapTracking,
                                     rpl->elfHeader.shnum * 8,
                                     4,
                                     &allocPtr,
                                     1,
                                     &allocSize,
                                     &largestFree,
                                     rpl->fileType);
   if (error != 0) {
      Loader_ReportError("***Allocate Error {}, Failed to allocate {} bytes for section addresses;  (needed {}, available {}).",
                         error,
                         rpl->elfHeader.shnum * 8,
                         rpl->sectionAddressBufferSize,
                         largestFree);
      goto lblError;
   }

   rpl->sectionAddressBuffer = virt_cast<virt_addr *>(allocPtr);
   rpl->sectionAddressBufferSize = allocSize;

   error = LiCacheLineCorrectAllocEx(dataHeapTracking,
                                     rpl->elfHeader.shnum * rpl->elfHeader.shentsize,
                                     4,
                                     &allocPtr,
                                     1,
                                     &allocSize,
                                     &largestFree,
                                     rpl->fileType);
   if (error != 0) {
      Loader_ReportError("*** Could not allocate space for section headers in {} heap;  (needed {}, available {})",
                         r9 ? "shared readonly" : "readonly",
                         rpl->sectionHeaderBufferSize,
                         largestFree);
      goto lblError;
   }

   rpl->sectionHeaderBuffer = allocPtr;
   rpl->sectionHeaderBufferSize = allocSize;

   std::memcpy(rpl->sectionHeaderBuffer.getRawPointer(),
               virt_cast<void *>(virt_cast<virt_addr>(fileElfHeader) + rpl->elfHeader.shoff).getRawPointer(),
               rpl->elfHeader.shnum * rpl->elfHeader.shentsize);

   if (allocModuleName) {
      error = LiCacheLineCorrectAllocEx(globals->processCodeHeap,
                                        align_up(moduleNameLen, 4),
                                        4,
                                        &allocPtr,
                                        1,
                                        &allocSize,
                                        &largestFree,
                                        rpl->fileType);
      if (error != 0) {
         Loader_ReportError("*** Could not allocate space for module name;  (needed {}, available {})\n",
                            rpl->moduleNameBufferSize,
                            largestFree);
         goto lblError;
      }

      rpl->moduleNameBuffer = virt_cast<char *>(allocPtr);
      rpl->moduleNameBufferSize = allocSize;

      std::strncpy(virt_cast<char *>(rpl->moduleNameBuffer).getRawPointer(),
                   moduleName.getRawPointer(),
                   rpl->moduleNameBufferSize);
   } else {
      rpl->moduleNameBuffer = moduleName;
      rpl->moduleNameBufferSize = moduleNameLen;
   }

   rpl->moduleNameLen = moduleNameLen;
   rpl->moduleNameBuffer[rpl->moduleNameLen] = char { 0 };

   // Load SHT_RPL_CRCS
   shRplCrcs = getSectionHeader(rpl, rpl->elfHeader.shnum - 2);
   if (shRplCrcs->type != rpl::SHT_RPL_CRCS ||
       (shRplCrcs->flags & rpl::SHF_DEFLATED)) {
      Loader_ReportError("***shnum-2 section type = 0x{:08X}, flags=0x{:08X}",
                         shRplCrcs->type, shRplCrcs->flags);
      LiSetFatalError(0x18729B, loadArgs->fileType, 1, "LiLoadRPLBasics", 0x403);
      error = -470081;
      goto lblError;
   }

   error = LiCacheLineCorrectAllocEx(globals->processCodeHeap,
                                     shRplCrcs->size,
                                     -static_cast<int32_t>(shRplCrcs->addralign),
                                     &allocPtr,
                                     1,
                                     &allocSize,
                                     &largestFree,
                                     rpl->fileType);
   if (error != 0) {
      Loader_ReportError("*** Could not allocate space for CRCs;  (needed {}, available {})",
                         rpl->crcBufferSize, largestFree);
      goto lblError;
   }

   rpl->crcBuffer = allocPtr;
   rpl->crcBufferSize = allocSize;

   sectionCrcs = virt_cast<uint32_t *>(rpl->crcBuffer);
   std::memcpy(rpl->crcBuffer.getRawPointer(),
               virt_cast<void *>(virt_cast<virt_addr>(fileElfHeader) + shRplCrcs->offset).getRawPointer(),
               shRplCrcs->size);

   rpl->sectionAddressBuffer[rpl->elfHeader.shnum - 2] =
      virt_cast<virt_addr>(rpl->crcBuffer);

   // Load SHT_RPL_FILEINFO
   shRplFileInfo = getSectionHeader(rpl, rpl->elfHeader.shnum - 1);
   if (shRplFileInfo->type != rpl::SHT_RPL_FILEINFO ||
      (shRplFileInfo->flags & rpl::SHF_DEFLATED)) {
      Loader_ReportError("***shnum-1 section type = 0x{:08X}, flags=0x{:08X}",
                         shRplFileInfo->type, shRplFileInfo->flags);
      LiSetFatalError(0x18729B, loadArgs->fileType, 1, "LiLoadRPLBasics", 0x41A);
      error = -470082;
      goto lblError;
   }

   rpl->fileInfoSize = shRplFileInfo->size;
   error = LiCacheLineCorrectAllocEx(dataHeapTracking,
                                     shRplFileInfo->size,
                                     shRplFileInfo->addralign,
                                     &allocPtr,
                                     1,
                                     &allocSize,
                                     &largestFree,
                                     rpl->fileType);
   if (error != 0) {
      Loader_ReportError("*** Could not allocate space for file info;  (needed {}, available {})",
                         rpl->fileInfoBufferSize, largestFree);
      goto lblError;
   }

   rpl->fileInfoBuffer = virt_cast<rpl::RPLFileInfo_v4_2 *>(allocPtr);
   rpl->fileInfoBufferSize = allocSize;

   Loader_ReportNotice("RPL_LAYOUT:%s,FILE,start,=\"0x{:08x}\"\nRPL_LAYOUT:{},FILE,end,=\"0x{:08x}\"",
                       rpl->moduleNameBuffer,
                       rpl->fileInfoBuffer,
                       rpl->moduleNameBuffer,
                       virt_cast<virt_addr>(rpl->fileInfoBuffer) + rpl->fileInfoBufferSize);

   std::memcpy(rpl->fileInfoBuffer.getRawPointer(),
               virt_cast<void *>(virt_cast<virt_addr>(fileElfHeader) + shRplFileInfo->offset).getRawPointer(),
               shRplFileInfo->size);

   rpl->sectionAddressBuffer[rpl->elfHeader.shnum - 1] =
      virt_cast<virt_addr>(rpl->fileInfoBuffer);

   fileInfoCrc = LiCalcCRC32(0, rpl->fileInfoBuffer, shRplFileInfo->size);
   if (fileInfoCrc != sectionCrcs[rpl->elfHeader.shnum - 1]) {
      Loader_ReportError("***FileInfo CRC failed check.");
      LiSetFatalError(0x18729B, loadArgs->fileType, 1, "LiLoadRPLBasics", 0x433);
      error = -470083;
      goto lblError;
   }

   rpl->fileInfoBuffer->tlsModuleIndex = int16_t { -1 };
   error = LiCheckFileBounds(rpl);
   if (error == 0) {
      rpl->virtualFileBase = fileElfHeader;
      *outLoadedRpl = rpl;
      return error;
   }

lblError:
   if (rpl) {
      if (rpl->fileInfoBuffer) {
         LiCacheLineCorrectFreeEx(dataHeapTracking,
                                  rpl->fileInfoBuffer,
                                  rpl->fileInfoBufferSize);
      }

      if (rpl->crcBuffer) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  rpl->crcBuffer,
                                  rpl->crcBufferSize);
      }

      if (rpl->moduleNameBuffer) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  rpl->moduleNameBuffer,
                                  rpl->moduleNameBufferSize);
      }

      if (rpl->sectionHeaderBuffer) {
         LiCacheLineCorrectFreeEx(dataHeapTracking,
                                  rpl->sectionHeaderBuffer,
                                  rpl->sectionHeaderBufferSize);
      }

      if (rpl->sectionAddressBuffer) {
         LiCacheLineCorrectFreeEx(dataHeapTracking,
                                  rpl->sectionAddressBuffer,
                                  rpl->sectionAddressBufferSize);
      }

      if (rpl->pathBuffer) {
         LiCacheLineCorrectFreeEx(dataHeapTracking,
                                  rpl->pathBuffer,
                                  rpl->pathBufferSize);
      }

      if (rpl->textBuffer) {
         LiCacheLineCorrectFreeEx(codeHeapTracking,
                                  rpl->textBuffer,
                                  rpl->textBufferSize);
      }

      LiCacheLineCorrectFreeEx(codeHeapTracking,
                               rpl,
                               rpl->selfBufferSize);
   }

   return error;
}

} // namespace cafe::loader::internal
