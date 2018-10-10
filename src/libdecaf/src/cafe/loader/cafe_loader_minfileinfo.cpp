#include "cafe_loader_error.h"
#include "cafe_loader_log.h"
#include "cafe_loader_loaded_rpl.h"
#include "cafe_loader_minfileinfo.h"

namespace cafe::loader::internal
{

bool
Loader_ValidateAddrRange(virt_addr addr,
                         uint32_t size)
{
   // TODO: Syscall to kernel validate addr range
   return TRUE;
}

int32_t
LiValidateAddress(virt_ptr<void> ptr,
                  uint32_t size,
                  uint32_t alignMask,
                  int32_t errorCode,
                  virt_addr minAddr,
                  virt_addr maxAddr,
                  std::string_view name)
{
   auto addr = virt_cast<virt_addr>(ptr);
   if (addr < minAddr || addr >= maxAddr) {
      if (!name.empty()) {
         Loader_ReportError("***bad {} address.", name);
      }
      return errorCode;
   }

   if (alignMask && (addr & alignMask)) {
      if (!name.empty()) {
         Loader_ReportError("***bad {} address alignment.", name);
      }

      return errorCode;
   }

   if (size && !Loader_ValidateAddrRange(addr, size)) {
      if (!name.empty()) {
         Loader_ReportError("***bad {} address buffer.", name);
      }

      return errorCode;
   }

   return 0;
}


/**
 * sUpdateFileInfoForUser
 */
void
updateFileInfoForUser(virt_ptr<LOADED_RPL> rpl,
                      virt_ptr<LOADER_UserFileInfo> userFileInfo,
                      virt_ptr<LOADER_MinFileInfo> minFileInfo)
{
   if (rpl->loadStateFlags & LoaderStateFlags_Unk0x20000000) {
      if (userFileInfo) {
         userFileInfo->fileInfoFlags |= rpl::RPL_FLAG_4;
      } else if (minFileInfo) {
         minFileInfo->fileInfoFlags |= rpl::RPL_FLAG_4;
      }
   }
}

int32_t
LiGetMinFileInfo(virt_ptr<LOADED_RPL> rpl,
                 virt_ptr<LOADER_MinFileInfo> info)
{
   auto fileInfo = rpl->fileInfoBuffer;
   *info->outSizeOfFileInfo = rpl->fileInfoSize;

   if (fileInfo->runtimeFileInfoSize) {
      *info->outSizeOfFileInfo = fileInfo->runtimeFileInfoSize;
   }

   info->dataSize = fileInfo->dataSize;
   info->dataAlign = fileInfo->dataAlign;
   info->loadSize = fileInfo->loadSize;
   info->loadAlign = fileInfo->loadAlign;
   info->fileInfoFlags = fileInfo->flags;

   if (rpl->fileType == ios::mcp::MCPFileType::ProcessCode) {
      info->fileLocation = getProcTitleLoc();
   }

   if (info->inoutNextTlsModuleNumber && (fileInfo->flags & 8)) {
      if (fileInfo->tlsModuleIndex != -1) {
         Loader_ReportError("*** unexpected module index.\n");
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LiGetMinFileInfo", 261);
         return -470064;
      }

      fileInfo->tlsModuleIndex = *info->inoutNextTlsModuleNumber;
      *info->inoutNextTlsModuleNumber += 1;
   }

   if (fileInfo->filename) {
      auto path = virt_cast<char *>(rpl->fileInfoBuffer) + fileInfo->filename;
      *info->outPathStringSize =
         static_cast<uint32_t>(strnlen(path.get(),
                                       rpl->fileInfoSize - fileInfo->filename)
                               + 1);
   } else {
      *info->outPathStringSize = 0u;
   }

   updateFileInfoForUser(rpl, nullptr, info);
   return 0;
}

int32_t
LiValidateMinFileInfo(virt_ptr<LOADER_MinFileInfo> minFileInfo,
                      std::string_view funcName)
{
   if (!minFileInfo) {
      Loader_ReportError("*** Null minimum file info pointer.");
      LiSetFatalError(0x18729Bu, 0, 1, "LiValidateMinFileInfo", 316);
      return -470058;
   }

   auto error = LiValidateAddress(minFileInfo,
                                  sizeof(LOADER_MinFileInfo),
                                  0,
                                  -470058,
                                  virt_addr { 0x10000000 },
                                  virt_addr { 0xC0000000 },
                                  "minimum file info");
   if (error) {
      LiSetFatalError(0x18729Bu, 0, 1, "LiValidateMinFileInfo", 325);
      return error;
   }

   if (minFileInfo->size != sizeof(LOADER_MinFileInfo)) {
      Loader_ReportError("***{} received invalid minimum file control block size.", funcName);
      LiSetFatalError(0x18729Bu, 0, 1, "LiValidateMinFileInfo", 334);
      return -470059;
   }

   if (minFileInfo->version != 4) {
      Loader_ReportError("***{} received invalid minimum file control block version.", funcName);
      LiSetFatalError(0x18729Bu, 0, 1, "LiValidateMinFileInfo", 342);
      return -470059;
   }

   if (minFileInfo->outKernelHandle) {
      error = LiValidateAddress(minFileInfo->outKernelHandle,
                                4,
                                0,
                                -470027,
                                virt_addr { 0x10000000 },
                                virt_addr { 0xC0000000 },
                                "kernel handle for module (out of valid range)");
      if (error) {
         LiSetFatalError(0x18729Bu, 0, 1, "LiValidateMinFileInfo", 353);
         return error;
      }
   }

   if (minFileInfo->moduleNameBuffer) {
      error = LiValidateAddress(minFileInfo->moduleNameBuffer,
                                minFileInfo->moduleNameBufferLen,
                                0,
                                -470027,
                                virt_addr { 0x10000000 },
                                virt_addr { 0xC0000000 },
                                "module name");
      if (error) {
         LiSetFatalError(0x18729Bu, 0, 1, "LiValidateMinFileInfo", 364);
         return error;
      }
   }

   if (minFileInfo->outNumberOfSections) {
      error = LiValidateAddress(minFileInfo->outNumberOfSections,
                                4,
                                0,
                                -470027,
                                virt_addr { 0x10000000 },
                                virt_addr { 0xC0000000 },
                                "number of sections");
      if (error) {
         LiSetFatalError(0x18729Bu, 0, 1, "LiValidateMinFileInfo", 375);
         return error;
      }
   }

   if (minFileInfo->outSizeOfFileInfo) {
      error = LiValidateAddress(minFileInfo->outSizeOfFileInfo,
                                4,
                                0,
                                -470027,
                                virt_addr { 0x10000000 },
                                virt_addr { 0xC0000000 },
                                "size of file info");
      if (error) {
         LiSetFatalError(0x18729Bu, 0, 1, "LiValidateMinFileInfo", 386);
         return error;
      }
   }

   if (minFileInfo->outPathStringSize) {
      error = LiValidateAddress(minFileInfo->outPathStringSize,
                                4,
                                0,
                                -470027,
                                virt_addr { 0x10000000 },
                                virt_addr { 0xC0000000 },
                                "size of path string");
      if (error) {
         LiSetFatalError(0x18729Bu, 0, 1, "LiValidateMinFileInfo", 397);
         return error;
      }
   }

   if (minFileInfo->inoutNextTlsModuleNumber) {
      error = LiValidateAddress(minFileInfo->inoutNextTlsModuleNumber,
                                4,
                                0,
                                -470027,
                                virt_addr { 0x10000000 },
                                virt_addr { 0xC0000000 },
                                "next TLS module number");
      if (error) {
         LiSetFatalError(0x18729Bu, 0, 1, "LiValidateMinFileInfo", 408);
         return error;
      }
   }

   return 0;
}

} // namespace cafe::loader::internal

