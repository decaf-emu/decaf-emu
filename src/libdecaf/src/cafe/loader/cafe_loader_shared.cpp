#include "cafe_loader_bounce.h"
#include "cafe_loader_entry.h"
#include "cafe_loader_error.h"
#include "cafe_loader_flush.h"
#include "cafe_loader_globals.h"
#include "cafe_loader_heap.h"
#include "cafe_loader_init.h"
#include "cafe_loader_iop.h"
#include "cafe_loader_basics.h"
#include "cafe_loader_log.h"
#include "cafe_loader_minfileinfo.h"
#include "cafe_loader_prep.h"
#include "cafe_loader_reloc.h"
#include "cafe_loader_setup.h"
#include "cafe_loader_shared.h"
#include "cafe_loader_utils.h"
#include "cafe/cafe_stackobject.h"

#include "cafe/libraries/cafe_hle_library.h"
#include <zlib.h>

namespace cafe::loader::internal
{

constexpr const char *
SharedLibraryList[] {
   "tve.rpl",
   "nsysccr.rpl",
   "nsysnet.rpl",
   "uvc.rpl",
   "tcl.rpl",
   "nn_pdm.rpl",
   "dmae.rpl",
   "dc.rpl",
   "vpadbase.rpl",
   "vpad.rpl",
   "avm.rpl",
   "gx2.rpl",
   "snd_core.rpl",
};

struct LiCompressedSharedDataTracking
{
   be2_virt_ptr<void> data;
   be2_virt_ptr<void> intialisationData;
   be2_val<uint32_t> compressedSize;
   be2_val<uint32_t> size;
};
CHECK_OFFSET(LiCompressedSharedDataTracking, 0x00, data);
CHECK_OFFSET(LiCompressedSharedDataTracking, 0x04, intialisationData);
CHECK_OFFSET(LiCompressedSharedDataTracking, 0x08, compressedSize);
CHECK_OFFSET(LiCompressedSharedDataTracking, 0x0C, size);
CHECK_SIZE(LiCompressedSharedDataTracking, 0x10);

struct LoaderShared
{
   be2_virt_ptr<LOADED_RPL> loadedModules;
   be2_virt_ptr<void> dataBufferHead;
   be2_val<uint32_t> numFreeTrackCompBlocks;
   be2_virt_ptr<LiCompressedSharedDataTracking> freeTrackCompBlocks;
   be2_val<uint32_t> numUsedTrackCompBlocks;
   be2_virt_ptr<LiCompressedSharedDataTracking> usedTrackCompBlocks;
};
CHECK_OFFSET(LoaderShared, 0x00, loadedModules);
CHECK_OFFSET(LoaderShared, 0x04, dataBufferHead);
CHECK_OFFSET(LoaderShared, 0x08, numFreeTrackCompBlocks);
CHECK_OFFSET(LoaderShared, 0x0C, freeTrackCompBlocks);
CHECK_OFFSET(LoaderShared, 0x10, numUsedTrackCompBlocks);
CHECK_OFFSET(LoaderShared, 0x14, usedTrackCompBlocks);
CHECK_SIZE(LoaderShared, 0x18);

static virt_ptr<LoaderShared> gpLoaderShared = nullptr;
static virt_ptr<TinyHeap> gpSharedCodeHeapTracking = nullptr;
static virt_ptr<TinyHeap> gpSharedReadHeapTracking = nullptr;
static virt_ptr<TinyHeap> sgpTrackComp = nullptr;
static std::array<uint8_t, 0x1FF8> sRelocBuffer;
static virt_ptr<void> sHleUnimplementedStubMemory = nullptr;
static uint32_t sHleUnimplementedStubMemorySize = 0;

static int32_t
sLoadOneShared(std::string_view filename)
{
   auto moduleName = StackObject<virt_ptr<char>> { };
   auto moduleNameLen = StackObject<uint32_t> { };
   auto fileNameBuffer = StackArray<char, 32> { };
   auto chunkSize = uint32_t { 0 };
   auto chunkBuffer = virt_ptr<void> { nullptr };
   auto rpl = virt_ptr<LOADED_RPL> { nullptr };
   auto rplBasicLoadArgs = LiBasicsLoadArgs { };
   auto error = int32_t { 0 };

   Loader_LogEntry(2, 0, 0, "sLoadOneShared  Loading Shared RPL {}", filename);
   Loader_LogEntry(2, 0, 0, "sLoadOneShared  LiSyncBnce start {}", filename);
   LiInitBuffer(false);

   error = LiBounceOneChunk(filename,
                            ios::mcp::MCPFileType::CafeOS,
                            kernel::UniqueProcessId::Kernel,
                            &chunkSize,
                            0,
                            1,
                            &chunkBuffer);
   if (error != 0) {
      Loader_ReportError("***LiBounceOneChunk failed loading \"{}\" of type {} at offset 0x{:08X} err={}",
                         filename, 1, 0, error);
      return error;
   }

   Loader_LogEntry(2, 0, 0, "sLoadOneShared  LiSyncBnce end {}", filename);

   std::memcpy(fileNameBuffer.get(),
               filename.data(),
               filename.size());
   fileNameBuffer[filename.size()] = 0;

   *moduleName = fileNameBuffer;
   *moduleNameLen = static_cast<uint32_t>(filename.size());
   LiResolveModuleName(moduleName, moduleNameLen);

   rplBasicLoadArgs.fileOffset = 0u;
   rplBasicLoadArgs.pathNameLen = static_cast<uint32_t>(filename.size());
   rplBasicLoadArgs.pathName = fileNameBuffer;
   rplBasicLoadArgs.chunkBuffer = chunkBuffer;
   rplBasicLoadArgs.chunkBufferSize = chunkSize;
   rplBasicLoadArgs.readHeapTracking = gpSharedReadHeapTracking;
   rplBasicLoadArgs.upid = kernel::UniqueProcessId::Kernel;
   rplBasicLoadArgs.fileType = ios::mcp::MCPFileType::CafeOS;

   error = LiLoadRPLBasics(*moduleName,
                           *moduleNameLen,
                           chunkBuffer,
                           gpSharedCodeHeapTracking,
                           gpSharedReadHeapTracking,
                           true, // TODO: Change to false and keep module name in loader .data memory
                           1,
                           &rpl,
                           &rplBasicLoadArgs,
                           0);
   if (error != 0) {
      return error;
   }

   rpl->loadStateFlags |= LoaderStateFlags_Unk0x20000000 | LoaderStateFlag4;

   auto fileInfo = rpl->fileInfoBuffer;
   if (fileInfo->dataSize) {
      auto dataBufferHeadAddr = virt_cast<virt_addr>(gpLoaderShared->dataBufferHead);

      // Align data buffer start
      dataBufferHeadAddr = align_up(dataBufferHeadAddr, fileInfo->dataAlign);
      rpl->dataBuffer = virt_cast<void *>(dataBufferHeadAddr);

      // Align data buffer end to 64 bytes
      dataBufferHeadAddr = align_up(dataBufferHeadAddr + fileInfo->dataSize, 64);
      gpLoaderShared->dataBufferHead = virt_cast<void *>(dataBufferHeadAddr);
   }

   if (fileInfo->loadSize != fileInfo->fileInfoPad) {
      auto allocPtr = virt_ptr<void> { nullptr };
      auto tinyHeapError = TinyHeap_Alloc(gpSharedReadHeapTracking,
                                          fileInfo->loadSize - fileInfo->fileInfoPad,
                                          -static_cast<int32_t>(fileInfo->loadAlign),
                                          &allocPtr);
      if (tinyHeapError != TinyHeapError::OK) {
         Loader_ReportError("Could not allocate read-only space for shared library \"{}\"",
                            rpl->moduleNameBuffer);
         return static_cast<int32_t>(tinyHeapError);
      }

      rpl->loadBuffer = allocPtr;
   }

   Loader_LogEntry(2, 0, 0, "sLoadOneShared  LiSetupOneRPL start.");
   error = LiSetupOneRPL(kernel::UniqueProcessId::Invalid,
                         rpl,
                         gpSharedCodeHeapTracking,
                         gpSharedReadHeapTracking);
   if (error) {
      Loader_ReportError("LiSetupOneRPL failed for shared library \"{}\".", filename);
      return error;
   }

   Loader_LogEntry(2, 0, 0, "sLoadOneShared  LiSetupOneRPL end.");

   if (!rpl->elfHeader.shstrndx) {
      Loader_ReportError(
         "*** Error: Could not get section string table index for \"{}\".",
         filename);
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sLoadOneShared", 213);
      return -470071;
   }

   if (!rpl->sectionAddressBuffer[rpl->elfHeader.shstrndx]) {
      Loader_ReportError("*** Error: Could not get section string table for \"%s\".",
                         filename);
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sLoadOneShared", 204);
      return -470072;
   }

   auto shStr = virt_cast<char *>(rpl->sectionAddressBuffer[rpl->elfHeader.shstrndx]);
   auto importTracking = virt_ptr<LiImportTracking> { nullptr };
   auto importTrackingSize = uint32_t { 0 };

   // Setup import tracking
   for (auto i = 1; i < rpl->elfHeader.shnum - 2; ++i) {
      auto sectionHeader = virt_cast<rpl::SectionHeader *>(
         virt_cast<virt_addr>(rpl->sectionHeaderBuffer) +
         i * rpl->elfHeader.shentsize);
      if (!sectionHeader->size || sectionHeader->type != rpl::SHT_RPL_IMPORTS) {
         continue;
      }

      // Make sure we have memory for import tracking
      if (!importTracking) {
         auto largestFree = uint32_t { 0 };
         error = LiCacheLineCorrectAllocEx(getGlobalStorage()->processCodeHeap,
                                           sizeof(LiImportTracking) * (rpl->elfHeader.shnum - 2),
                                           4,
                                           reinterpret_cast<virt_ptr<void> *>(&importTracking),
                                           1,
                                           &importTrackingSize,
                                           &largestFree,
                                           rpl->fileType);
         if (error) {
            Loader_ReportError(
               "***Could not allocate space for shared import tracking in local heap;  (needed {}, available {}).",
               importTrackingSize, largestFree);
            return error;
         }
      }

      // Load imported rpl
      auto name = shStr + sectionHeader->name + strlen(".fimport_");
      auto nameLen = strlen(name.get());
      auto importModule = virt_ptr<LOADED_RPL> { nullptr };

      for (auto module = gpLoaderShared->loadedModules; module; module = module->nextLoadedRpl) {
         if (module->moduleNameLen == nameLen &&
             strncmp(name.get(),
                     module->moduleNameBuffer.get(),
                     nameLen) == 0) {
            importModule = module;
            break;
         }
      }

      if (!importModule) {
         Loader_ReportError(
            "*** \"{}\" imports from \"{}\" which is not loaded as a shared library.",
            filename, name);
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sLoadOneShared", 253);
         LiCacheLineCorrectFreeEx(getGlobalStorage()->processCodeHeap, importTracking, importTrackingSize);
         return -470010;
      }

      if (sectionHeader->flags & rpl::SHF_EXECINSTR) {
         importTracking[i].numExports = importModule->numFuncExports;
         importTracking[i].exports = virt_cast<rpl::Export *>(importModule->funcExports);
      } else {
         importTracking[i].numExports = importModule->numDataExports;
         importTracking[i].exports = virt_cast<rpl::Export *>(importModule->dataExports);
      }

      importTracking[i].tlsModuleIndex = importModule->fileInfoBuffer->tlsModuleIndex;
      importTracking[i].rpl = rpl;
   }

   // Process relocations and imports
   Loader_LogEntry(2, 0, 0, "sLoadOneShared  LiFixupRelocOneRPL start.");
   error = LiFixupRelocOneRPL(rpl, importTracking, 0);
   Loader_LogEntry(2, 0, 0, "sLoadOneShared  LiFixupRelocOneRPL end.");
   if (error) {
      Loader_ReportError("LiFixupRelocOneRPL failed for shared library \"{}\".", filename);
      return error;
   }

   for (auto i = 1; i < rpl->elfHeader.shnum - 2; ++i) {
      auto sectionHeader = getSectionHeader(rpl, i);
      if (sectionHeader->size && sectionHeader->type == rpl::SHT_NOBITS) {
         auto tinyHeapError =
            TinyHeap_AllocAt(sgpTrackComp,
                             virt_cast<void *>(rpl->sectionAddressBuffer[i]),
                             sectionHeader->size);
         if (tinyHeapError != TinyHeapError::OK) {
            Loader_Panic(0x13000B, "*** Critical error in tracking shared bss.");
         }
      }
   }

   if (rpl->compressedRelocationsBuffer) {
      LiCacheLineCorrectFreeEx(gpSharedCodeHeapTracking,
                               rpl->compressedRelocationsBuffer,
                               rpl->compressedRelocationsBufferSize);
      rpl->compressedRelocationsBuffer = nullptr;
   }

   if (importTracking) {
      LiCacheLineCorrectFreeEx(getGlobalStorage()->processCodeHeap, importTracking, importTrackingSize);
      importTracking = nullptr;
   }

   LiCacheLineCorrectFreeEx(getGlobalStorage()->processCodeHeap, rpl->crcBuffer, rpl->crcBufferSize);
   rpl->crcBuffer = nullptr;
   rpl->crcBufferSize = 0u;
   rpl->sectionAddressBuffer[rpl->elfHeader.shnum - 2] = virt_addr { 0 };
   rpl->nextLoadedRpl = gpLoaderShared->loadedModules;
   gpLoaderShared->loadedModules = rpl;
   return 0;
}

static int32_t
LiInitSharedForAll()
{
   Loader_LogEntry(2, 1, 512, "LiInitSharedForAll");

   // Setup tracking compression block heap
   auto outAllocPtr = virt_ptr<void> { nullptr };
   auto tinyHeapError = TinyHeap_Alloc(gpSharedCodeHeapTracking,
                                       0x430, -4,
                                       &outAllocPtr);
   if (tinyHeapError < TinyHeapError::OK) {
      Loader_Panic(0x13000C, "***Could not allocate memory for tracking compression blocks for shared data.");
   }

   sgpTrackComp = virt_cast<TinyHeap *>(outAllocPtr);
   tinyHeapError = TinyHeap_Setup(sgpTrackComp, 0x430, virt_cast<void *>(virt_addr { 0x10000000 }), 0x60000000); // 1536 mb??
   if (tinyHeapError < TinyHeapError::OK) {
      Loader_Panic(0x13000E, "***Could not setup heap for tracking compression blocks for shared data.");
   }

   // First load coreinit.rpl
   auto error = sLoadOneShared("coreinit.rpl");
   if (error) {
      Loader_Panic(0x13000F, "***Could not bounceload coreinit.rpl to shared code area.");
   }

   // Load remaining shared libraries
   Loader_LogEntry(2, 1, 0, "LiInitSharedForAll  load GRP start");
   for (auto &name : SharedLibraryList) {
      error = sLoadOneShared(name);
      if (error) {
         Loader_Panic(0x130010, "*** Could not bounceload shared library to shared code area.");
      }
   }
   Loader_LogEntry(2, 1, 0, "LiInitSharedForAll  load GRP end");

   gpLoaderShared->dataBufferHead = align_up(gpLoaderShared->dataBufferHead, 0x100);

   // Count number of track comp blocks
   for (auto block = TinyHeap_Enum(sgpTrackComp, nullptr, nullptr, nullptr);
        block;
        block = TinyHeap_Enum(sgpTrackComp, block, nullptr, nullptr)) {
      ++gpLoaderShared->numUsedTrackCompBlocks;
   }

   for (auto block = TinyHeap_EnumFree(sgpTrackComp, nullptr, nullptr, nullptr);
        block;
        block = TinyHeap_EnumFree(sgpTrackComp, block, nullptr, nullptr)) {
      ++gpLoaderShared->numFreeTrackCompBlocks;
   }

   tinyHeapError =
      TinyHeap_Alloc(gpSharedReadHeapTracking,
                     static_cast<int32_t>(sizeof(LiCompressedSharedDataTracking) * (gpLoaderShared->numFreeTrackCompBlocks + gpLoaderShared->numUsedTrackCompBlocks)),
                     -4,
                     &outAllocPtr);
   if (tinyHeapError != TinyHeapError::OK) {
      Loader_Panic(0x130011, "***Coult not allocate enough space for compressed shared data tracking.");
   }

   gpLoaderShared->freeTrackCompBlocks =
      virt_cast<LiCompressedSharedDataTracking *>(outAllocPtr);
   gpLoaderShared->usedTrackCompBlocks =
      virt_cast<LiCompressedSharedDataTracking *>(
         virt_cast<virt_addr>(outAllocPtr)
         + (sizeof(LiCompressedSharedDataTracking) * gpLoaderShared->numFreeTrackCompBlocks));

   auto blockIndex = 0u;
   auto blockPointer = virt_ptr<void> { 0 };
   auto blockSize = uint32_t { 0 };
   for (auto block = TinyHeap_Enum(sgpTrackComp, nullptr, &blockPointer, &blockSize);
        block;
        block = TinyHeap_Enum(sgpTrackComp, block, &blockPointer, &blockSize)) {
      gpLoaderShared->usedTrackCompBlocks[blockIndex].data = blockPointer;
      gpLoaderShared->usedTrackCompBlocks[blockIndex].intialisationData = nullptr;
      gpLoaderShared->usedTrackCompBlocks[blockIndex].compressedSize = 0u;
      gpLoaderShared->usedTrackCompBlocks[blockIndex].size = blockSize;
      ++blockIndex;
   }

   blockIndex = 0u;
   for (auto block = TinyHeap_EnumFree(sgpTrackComp, nullptr, &blockPointer, &blockSize);
        block;
        block = TinyHeap_EnumFree(sgpTrackComp, block, &blockPointer, &blockSize)) {
      gpLoaderShared->freeTrackCompBlocks[blockIndex].data = blockPointer;
      gpLoaderShared->freeTrackCompBlocks[blockIndex].intialisationData = nullptr;
      gpLoaderShared->freeTrackCompBlocks[blockIndex].compressedSize = 0u;
      gpLoaderShared->freeTrackCompBlocks[blockIndex].size = blockSize;
      ++blockIndex;
   }

   --gpLoaderShared->numFreeTrackCompBlocks; // Why? i do not know.

   // Allocate temporary buffer to use for compressing free blocks
   auto compressedInitialisationData = virt_ptr<void> { nullptr };
   tinyHeapError =  TinyHeap_Alloc(gpSharedCodeHeapTracking,
                                   TinyHeap_GetLargestFree(gpSharedCodeHeapTracking) - 4,
                                   4,
                                   &compressedInitialisationData);
   if (tinyHeapError != TinyHeapError::OK) {
      Loader_Panic(0x130012, "***Could not allocate space for compressed initialization data.");
   }

   for (auto i = 0u; i < gpLoaderShared->numFreeTrackCompBlocks; ++i) {
      auto &block = gpLoaderShared->freeTrackCompBlocks[i];
      auto compressedBlockSize = uLongf { block.size };
      if (block.size > 0x200) {
         error = compress(reinterpret_cast<Bytef *>(compressedInitialisationData.get()),
                          &compressedBlockSize,
                          reinterpret_cast<Bytef *>(block.data.get()),
                          block.size);
         if (error != Z_OK) {
            if (error == Z_MEM_ERROR) {
               error = Error::ZlibMemError;
               LiSetFatalError(0x187298u, 1u, 0, "LiInitSharedForAll", 487);
            } else if (error == Z_BUF_ERROR) {
               error = Error::ZlibBufError;
               LiSetFatalError(0x18729Bu, 1u, 1, "LiInitSharedForAll", 483);
            }

            Loader_Panic(0x130013, "***Could not compress initialization data for processes shared libraries.");
         }
      }

      if (((100 * compressedBlockSize) / block.size) <= 90) {
         // Stored the compressed initialisation data when compressed size < 90%
         tinyHeapError = TinyHeap_Alloc(gpSharedReadHeapTracking,
                                        static_cast<int32_t>(compressedBlockSize),
                                        -4,
                                        &outAllocPtr);
         if (tinyHeapError != TinyHeapError::OK) {
            Loader_Panic(0x130015, "***Could not allocate space for compressed shared initialization data.");
            error = static_cast<int32_t>(tinyHeapError);
            break;
         }

         block.intialisationData = outAllocPtr;
         block.compressedSize = static_cast<uint32_t>(compressedBlockSize);

         std::memcpy(block.intialisationData.get(),
                     compressedInitialisationData.get(),
                     compressedBlockSize);
         Loader_FlushDataRangeNoSync(virt_cast<virt_addr>(block.intialisationData),
                                     block.compressedSize);
      } else {
         // Store uncompressed
         tinyHeapError = TinyHeap_Alloc(gpSharedReadHeapTracking,
                                        block.size,
                                        -4,
                                        &outAllocPtr);
         if (tinyHeapError != TinyHeapError::OK) {
            Loader_Panic(0x130015, "***Could not allocate space for compressed shared initialization data.");
            error = static_cast<int32_t>(tinyHeapError);
            break;
         }

         block.intialisationData = outAllocPtr;
         std::memcpy(block.intialisationData.get(),
                     block.data.get(),
                     block.size);
         Loader_FlushDataRangeNoSync(virt_cast<virt_addr>(block.intialisationData),
                                     block.size);
      }
   }

   TinyHeap_Free(gpSharedCodeHeapTracking, compressedInitialisationData);
   TinyHeap_Free(gpSharedCodeHeapTracking, sgpTrackComp);
   Loader_LogEntry(2, 1, 1024, "LiInitSharedForAll");
   return error;
}

int32_t
initialiseSharedHeaps()
{
   constexpr auto SharedCodeTrackingSize = 0x830u;
   constexpr auto SharedReadTrackingSize = 0x1030u;
   constexpr auto LoaderSharedAddr = virt_addr { 0xFA000000 };
   constexpr auto SharedCodeHeapTrackingAddr = LoaderSharedAddr + sizeof(LoaderShared);
   constexpr auto SharedReadHeapTrackingAddr = SharedCodeHeapTrackingAddr + SharedCodeTrackingSize;

   constexpr auto SharedCodeHeapAddr = virt_addr { 0x01000000 };
   constexpr auto SharedCodeHeapSize = uint32_t { 0x007E0000 };

   constexpr auto SharedReadHeapAddr = virt_addr { 0xF8000000 };
   constexpr auto SharedReadHeapSize = uint32_t { 0x03000000 };
   constexpr auto SharedReadHeapReserveSize = uint32_t { 0x02000000 }; // Unknown

   gpLoaderShared = virt_cast<LoaderShared *>(LoaderSharedAddr);
   gpSharedCodeHeapTracking = virt_cast<TinyHeap *>(SharedCodeHeapTrackingAddr);
   gpSharedReadHeapTracking = virt_cast<TinyHeap *>(SharedReadHeapTrackingAddr);

   if (getProcFlags().isFirstProcess()) {
      if (TinyHeap_Setup(gpSharedCodeHeapTracking,
                         SharedCodeTrackingSize,
                         virt_cast<void *>(SharedCodeHeapAddr),
                         SharedCodeHeapSize) != TinyHeapError::OK) {
         Loader_Panic(0x130002, "***Could not initialize shared code heap tracking.");
      }

      if (TinyHeap_Setup(gpSharedReadHeapTracking,
                         SharedReadTrackingSize,
                         virt_cast<void *>(SharedReadHeapAddr),
                         SharedReadHeapSize) != TinyHeapError::OK) {
         Loader_Panic(0x130003, "***Could not initialize data heap tracking.");
      }

      // Reserve space for loader .text section
      if (TinyHeap_AllocAt(gpSharedCodeHeapTracking,
                           virt_cast<void *>(SharedCodeHeapAddr),
                           align_up(0x1C758, 1024)) != TinyHeapError::OK) {
         Loader_Panic(0x130004, "***Could not reserve shared code space for loader.");
      }

      // Reserve unknown chunk in shared read heap
      if (TinyHeap_AllocAt(gpSharedReadHeapTracking,
                           virt_cast<void *>(SharedReadHeapAddr),
                           SharedReadHeapReserveSize) != TinyHeapError::OK) {
         Loader_Panic(0x130005, "***Could not reserve read/only space for heap tracking.");
      }

      // Reserve gpLoaderShared
      if (TinyHeap_AllocAt(gpSharedReadHeapTracking,
                           gpLoaderShared,
                           sizeof(LoaderShared)) != TinyHeapError::OK) {
         Loader_Panic(0x130006, "***Could not reserve read/only space for heap tracking.");
      }

      // Reserve gpSharedCodeHeapTracking and gpSharedReadHeapTracking
      if (TinyHeap_AllocAt(gpSharedReadHeapTracking,
                           virt_cast<void *>(SharedCodeHeapTrackingAddr),
                           SharedCodeTrackingSize + SharedReadTrackingSize) != TinyHeapError::OK) {
         Loader_Panic(0x130007, "***Could not reserve read/only space for heap tracking.");
      }

      // Clear gpLoaderShared
      std::memset(gpLoaderShared.get(), 0, sizeof(LoaderShared));
      gpLoaderShared->dataBufferHead = virt_cast<void *>(virt_addr { 0x10000000 });
      Loader_ReportWarn("Title Loc is {}", getProcTitleLoc());
   }

   return 0;
}

virt_ptr<LOADED_RPL>
findLoadedSharedModule(std::string_view name)
{
   for (auto module = gpLoaderShared->loadedModules; module; module = module->nextLoadedRpl) {
      auto moduleName =
         std::string_view {
            module->moduleNameBuffer.get(),
            module->moduleNameLen
         };
      if (moduleName == name) {
         return module;
      }
   }

   return nullptr;
}

int32_t
LiInitSharedForProcess(virt_ptr<RPL_STARTINFO> initData)
{
   auto globals = getGlobalStorage();

   Loader_LogEntry(2, 1, 512, "LiInitSharedForProcess");
   if (globals->currentUpid != kernel::UniqueProcessId::Root) {
      if (getProcFlags().disableSharedLibraries()) {
         initData->dataAreaStart = 0x10000000u;
         Loader_ReportWarn("Shared Libraries disabled in process {}",
                           static_cast<int>(globals->currentUpid.value()));
         Loader_LogEntry(2, 1, 1024, "LiInitSharedForProcess");
         return 0;
      }
   }

   if (getProcFlags().isFirstProcess()) {
      auto error = LiInitSharedForAll();
      if (!error) {
         initData->dataAreaStart = virt_cast<virt_addr>(gpLoaderShared->dataBufferHead);

         // Allocate some memory to place HLE unimplemented function call stubs
         sHleUnimplementedStubMemorySize =
            TinyHeap_GetLargestFree(gpSharedCodeHeapTracking) - 4;
         TinyHeap_Alloc(gpSharedCodeHeapTracking,
                        sHleUnimplementedStubMemorySize,
                        4,
                        &sHleUnimplementedStubMemory);

         hle::setUnimplementedFunctionStubMemory(sHleUnimplementedStubMemory,
                                                 sHleUnimplementedStubMemorySize);
      }

      Loader_LogEntry(2, 1, 1024, "LiInitSharedForProcess");
      return error;
   }

   // TODO: Finish LiInitSharedForProcess for non-first processes
   decaf_abort("LiInitSharedForProcess not implemented for not first process");
}

} // namespace cafe::loader::internal
