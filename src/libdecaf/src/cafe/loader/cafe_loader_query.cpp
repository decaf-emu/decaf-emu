#include "cafe_loader_bounce.h"
#include "cafe_loader_error.h"
#include "cafe_loader_log.h"
#include "cafe_loader_loaded_rpl.h"
#include "cafe_loader_globals.h"
#include "cafe_loader_minfileinfo.h"
#include "cafe_loader_utils.h"
#include "cafe_loader_query.h"

#include <common/strutils.h>
#include <cstring>

namespace cafe::loader::internal
{

virt_ptr<LOADED_RPL>
getModule(LOADER_Handle handle)
{
   auto globals = getGlobalStorage();
   for (auto rpl = globals->firstLoadedRpl; rpl; rpl = rpl->nextLoadedRpl) {
      if (rpl->moduleNameBuffer == handle) {
         return rpl;
      }
   }

   return nullptr;
}

int32_t
LOADER_GetSecInfo(kernel::UniqueProcessId upid,
                  LOADER_Handle handle,
                  virt_ptr<uint32_t> outNumberOfSections,
                  virt_ptr<LOADER_SectionInfo> outSectionInfo)
{
   auto globals = getGlobalStorage();
   if (globals->currentUpid != upid) {
      Loader_ReportError("*** Loader address space not set for process {} but called for {}.",
                         static_cast<int>(globals->currentUpid.value()),
                         static_cast<int>(upid));
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetSecInfo", 45);
      LiCloseBufferIfError();
      return Error::DifferentProcess;
   }

   // Verify outNumberOfSections pointer
   if (auto error = LiValidateAddress(outNumberOfSections,
                                      sizeof(uint32_t),
                                      0, -470009,
                                      virt_addr { 0x10000000 },
                                      virt_addr { 0xC0000000 },
                                      "number of sections")) {
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetSecInfo", 56);
      LiCloseBufferIfError();
      return error;
   }

   // Find the module
   if (!handle) {
      handle = globals->loadedRpx->moduleNameBuffer;
   }

   auto rpl = getModule(handle);
   if (!rpl) {
      Loader_ReportError("*** module not found.");
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetSecInfo", 75);
      LiCloseBufferIfError();
      return -470010;
   }

   // Check section count
   if (!outSectionInfo || !*outNumberOfSections) {
      *outNumberOfSections = rpl->elfHeader.shnum;
      return 0;
   }

   if (*outNumberOfSections < rpl->elfHeader.shnum) {
      Loader_ReportError("*** num sections is too small.\n");
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LOADER_GetSecInfo", 91);
      LiCloseBufferIfError();
      return -470011;
   }

   *outNumberOfSections = rpl->elfHeader.shnum;

   // Verify the outSectionInfo pointer
   if (auto error = LiValidateAddress(outSectionInfo,
                                      sizeof(LOADER_SectionInfo) * rpl->elfHeader.shnum,
                                      0, -470012,
                                      virt_addr { 0x10000000 },
                                      virt_addr { 0xC0000000 },
                                      "section return")) {
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetSecInfo", 56);
      LiCloseBufferIfError();
      return error;
   }

   // Fill out the section info data
   std::memset(outSectionInfo.get(),
               0,
               sizeof(LOADER_SectionInfo) * rpl->elfHeader.shnum);

   for (auto i = 1; i < rpl->elfHeader.shnum; ++i) {
      auto sectionHeader = getSectionHeader(rpl, i);
      if (sectionHeader->type == rpl::SHT_RELA ||
          sectionHeader->type == rpl::SHT_RPL_FILEINFO ||
          sectionHeader->type == rpl::SHT_RPL_CRCS) {
         continue;
      }

      auto &info = outSectionInfo[i];
      info.type = sectionHeader->type;
      info.flags = sectionHeader->flags;
      info.address = rpl->sectionAddressBuffer[i];
      if (sectionHeader->type == rpl::SHT_RPL_IMPORTS) {
         info.name = sectionHeader->name;
      } else {
         info.size = sectionHeader->size;
      }
   }

   return 0;
}

int32_t
LOADER_GetFileInfo(kernel::UniqueProcessId upid,
                   LOADER_Handle handle,
                   virt_ptr<uint32_t> outSizeOfFileInfo,
                   virt_ptr<LOADER_UserFileInfo> outFileInfo,
                   virt_ptr<uint32_t> nextTlsModuleNumber,
                   virt_ptr<uint32_t> outFileLocation)
{
   auto globals = getGlobalStorage();
   if (globals->currentUpid != upid) {
      Loader_ReportError("*** Loader address space not set for process {} but called for {}.",
                         static_cast<int>(globals->currentUpid.value()),
                         static_cast<int>(upid));
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetFileInfo", 59);
      LiCloseBufferIfError();
      return Error::DifferentProcess;
   }

   // Verify outSizeOfFileInfo pointer
   if (auto error = LiValidateAddress(outSizeOfFileInfo,
                                      sizeof(uint32_t),
                                      0, -470060,
                                      virt_addr { 0x10000000 },
                                      virt_addr { 0xC0000000 },
                                      "file info bytes")) {
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetFileInfo", 70);
      LiCloseBufferIfError();
      return error;
   }

   // Find the module
   if (!handle) {
      handle = globals->loadedRpx->moduleNameBuffer;
   }

   auto rpl = getModule(handle);
   if (!rpl) {
      Loader_ReportError("*** module not found.");
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetFileInfo", 89);
      LiCloseBufferIfError();
      return -470010;
   }

   // Check file info size
   auto fileInfoSize = rpl->fileInfoSize;
   auto fileInfo = rpl->fileInfoBuffer;
   if (fileInfo->runtimeFileInfoSize) {
      fileInfoSize = fileInfo->runtimeFileInfoSize;
   }

   rpl->userFileInfoSize = fileInfoSize;

   if (!outFileInfo || !*outSizeOfFileInfo) {
      *outSizeOfFileInfo = fileInfoSize;

      if (outFileLocation) {
         if (rpl->fileType == ios::mcp::MCPFileType::ProcessCode) {
            *outFileLocation = getProcTitleLoc();
         } else {
            *outFileLocation = 0u;
         }
      }

      return 0;
   }

   if (*outSizeOfFileInfo < fileInfoSize) {
      Loader_ReportError("*** file info size is too small.\n");
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LOADER_GetFileInfo", 124);
      LiCloseBufferIfError();
      return -470066;
   }

   *outSizeOfFileInfo = fileInfoSize;

   // Update the TLS module number if necessary
   if (nextTlsModuleNumber) {
      if (auto error = LiValidateAddress(nextTlsModuleNumber,
                                         sizeof(uint32_t),
                                         0, -470097,
                                         virt_addr { 0x10000000 },
                                         virt_addr { 0xC0000000 },
                                         "next TLS number")) {
         LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetFileInfo", 137);
         LiCloseBufferIfError();
         return error;
      }

      if (fileInfo->flags & rpl::RPL_HAS_TLS) {
         if (fileInfo->tlsModuleIndex != -1) {
            Loader_ReportError("*** unexpected module index.\n");
            LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LOADER_GetFileInfo", 152);
            LiCloseBufferIfError();
            return -470064;
         }

         fileInfo->tlsModuleIndex = static_cast<int16_t>((*nextTlsModuleNumber)++);
      }
   }

   // Verify the outFileInfo pointer
   if (auto error = LiValidateAddress(outFileInfo,
                                      sizeof(LOADER_UserFileInfo),
                                      0, -470062,
                                      virt_addr { 0x10000000 },
                                      virt_addr { 0xC0000000 },
                                      "file info return")) {
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetFileInfo", 165);
      LiCloseBufferIfError();
      return error;
   }

   // Set the file info
   rpl->userFileInfo = outFileInfo;
   outFileInfo->size = static_cast<uint32_t>(sizeof(LOADER_UserFileInfo));
   outFileInfo->magic = 0xCABE0402u;
   outFileInfo->pathStringLength = 0u;
   outFileInfo->pathString = nullptr;
   outFileInfo->fileInfoFlags = fileInfo->flags;
   outFileInfo->tlsAlignShift = fileInfo->tlsAlignShift;
   outFileInfo->tlsModuleIndex = fileInfo->tlsModuleIndex;
   outFileInfo->shstrndx = rpl->elfHeader.shstrndx;

   if (rpl->fileType == ios::mcp::MCPFileType::ProcessCode) {
      outFileInfo->titleLocation = getProcTitleLoc();
   } else {
      outFileInfo->titleLocation = 0u;
   }

   updateFileInfoForUser(rpl, outFileInfo, nullptr);
   return 0;
}

int32_t
LOADER_GetPathString(kernel::UniqueProcessId upid,
                     LOADER_Handle handle,
                     virt_ptr<uint32_t> outPathStringSize,
                     virt_ptr<char> pathStringBuffer,
                     virt_ptr<LOADER_UserFileInfo> outFileInfo)
{
   auto globals = getGlobalStorage();
   if (globals->currentUpid != upid) {
      Loader_ReportError("*** Loader address space not set for process {} but called for {}.",
                         static_cast<int>(globals->currentUpid.value()),
                         static_cast<int>(upid));
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetPathString", 304);
      LiCloseBufferIfError();
      return Error::DifferentProcess;
   }

   // Verify outPathStringSize pointer
   if (auto error = LiValidateAddress(outPathStringSize,
                                      sizeof(uint32_t),
                                      0, -470098,
                                      virt_addr { 0x10000000 },
                                      virt_addr { 0xC0000000 },
                                      "file info bytes")) { // Yes they copy and pasted this in loader.elf
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetPathString", 315);
      LiCloseBufferIfError();
      return error;
   }

   // Find the module
   if (!handle) {
      handle = globals->loadedRpx->moduleNameBuffer;
   }

   auto rpl = getModule(handle);
   if (!rpl) {
      Loader_ReportError("*** module not found.");
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetPathString", 334);
      LiCloseBufferIfError();
      return -470010;
   }

   // Check path string size
   if (!rpl->fileInfoBuffer || !rpl->fileInfoBuffer->filename) {
      *outPathStringSize = 0u;
      return 0;
   }

   auto path = virt_cast<char *>(rpl->fileInfoBuffer) + rpl->fileInfoBuffer->filename;
   auto pathLength = static_cast<uint32_t>(
      strnlen(path.get(),
              rpl->fileInfoBufferSize - rpl->fileInfoBuffer->filename) + 1);

   if (!pathStringBuffer ||
       *outPathStringSize == 0 ||
       pathLength == 0) {
      *outPathStringSize = pathLength;
      return 0;
   }

   if (*outPathStringSize < pathLength) {
      Loader_ReportError("*** notify path size is too small.\n");
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LOADER_GetPathString", 354);
      return -470068;
   }

   *outPathStringSize = pathLength;

   // Verify the pathStringBuffer pointer
   if (auto error = LiValidateAddress(pathStringBuffer,
                                      pathLength,
                                      0, -470099,
                                      virt_addr { 0x10000000 },
                                      virt_addr { 0xC0000000 },
                                      "path string return")) {
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_GetPathString", 366);
      LiCloseBufferIfError();
      return error;
   }

   // Copy the path string
   string_copy(pathStringBuffer.get(),
               path.get(),
               pathLength);
   pathStringBuffer[pathLength - 1] = char { 0 };

   if (outFileInfo) {
      outFileInfo->pathString = pathStringBuffer;
      outFileInfo->pathStringLength = pathLength;
   }

   return 0;
}

int32_t
LOADER_Query(kernel::UniqueProcessId upid,
             LOADER_Handle handle,
             virt_ptr<LOADER_MinFileInfo> minFileInfo)
{
   auto globals = getGlobalStorage();
   if (globals->currentUpid != upid) {
      Loader_ReportError("*** Loader address space not set for process {} but called for {}.",
                         static_cast<int>(globals->currentUpid.value()),
                         static_cast<int>(upid));
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_Query", 40);
      return Error::DifferentProcess;
   }

   if (auto error = LiValidateMinFileInfo(minFileInfo, "LOADER_Query")) {
      return error;
   }

   if (minFileInfo->outNumberOfSections) {
      if (auto error = LOADER_GetSecInfo(upid, handle,
                                         minFileInfo->outNumberOfSections,
                                         minFileInfo->outSectionInfo)) {
         return error;
      }
   }

   if (minFileInfo->outSizeOfFileInfo) {
      if (auto error = LOADER_GetFileInfo(upid, handle,
                                          minFileInfo->outSizeOfFileInfo,
                                          minFileInfo->outFileInfo,
                                          minFileInfo->inoutNextTlsModuleNumber,
                                          virt_addrof(minFileInfo->fileLocation))) {
         return error;
      }
   }

   if (minFileInfo->outPathStringSize) {
      if (auto error = LOADER_GetPathString(upid, handle,
                                            minFileInfo->outPathStringSize,
                                            minFileInfo->pathStringBuffer,
                                            minFileInfo->outFileInfo)) {
         return error;
      }
   }

   return 0;
}

} // namespace cafe::loader::internal
