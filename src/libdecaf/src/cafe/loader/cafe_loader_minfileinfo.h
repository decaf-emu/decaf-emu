#pragma once
#include "cafe_loader_basics.h"
#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::loader
{

//! Unique pointer representing a module handle, we actually use the pointer
//! value of the LOADED_RPL.moduleNameBuffer
using LOADER_Handle = virt_ptr<void>;

struct LOADER_LinkModule
{
   be2_val<LOADER_Handle> loaderHandle;
   be2_val<virt_addr> entryPoint;
   be2_val<virt_addr> textAddr;
   be2_val<uint32_t> textOffset;
   be2_val<uint32_t> textSize;
   be2_val<virt_addr> dataAddr;
   be2_val<uint32_t> dataOffset;
   be2_val<uint32_t> dataSize;
   be2_val<virt_addr> loadAddr;
   be2_val<uint32_t> loadOffset;
   be2_val<uint32_t> loadSize;
};
CHECK_OFFSET(LOADER_LinkModule, 0x00, loaderHandle);
CHECK_OFFSET(LOADER_LinkModule, 0x04, entryPoint);
CHECK_OFFSET(LOADER_LinkModule, 0x08, textAddr);
CHECK_OFFSET(LOADER_LinkModule, 0x0C, textOffset);
CHECK_OFFSET(LOADER_LinkModule, 0x10, textSize);
CHECK_OFFSET(LOADER_LinkModule, 0x14, dataAddr);
CHECK_OFFSET(LOADER_LinkModule, 0x18, dataOffset);
CHECK_OFFSET(LOADER_LinkModule, 0x1C, dataSize);
CHECK_OFFSET(LOADER_LinkModule, 0x20, loadAddr);
CHECK_OFFSET(LOADER_LinkModule, 0x24, loadOffset);
CHECK_OFFSET(LOADER_LinkModule, 0x28, loadSize);
CHECK_SIZE(LOADER_LinkModule, 0x2C);

struct LOADER_LinkInfo
{
   be2_val<uint32_t> size;
   be2_val<uint32_t> numModules;
   be2_array<LOADER_LinkModule, 1> modules;
};
CHECK_OFFSET(LOADER_LinkInfo, 0x00, size);
CHECK_OFFSET(LOADER_LinkInfo, 0x04, numModules);
CHECK_OFFSET(LOADER_LinkInfo, 0x08, modules);
CHECK_SIZE(LOADER_LinkInfo, 0x34);

struct LOADER_SectionInfo
{
   be2_val<uint32_t> type;
   be2_val<uint32_t> flags;
   be2_val<virt_addr> address;

   union
   {
      //! Size of the section, set when type != SHT_RPL_IMPORTS
      be2_val<uint32_t> size;

      //! Name offset of the section, set when type == SHT_RPL_IMPORTS
      be2_val<uint32_t> name;
   };
};
CHECK_OFFSET(LOADER_SectionInfo, 0x00, type);
CHECK_OFFSET(LOADER_SectionInfo, 0x04, flags);
CHECK_OFFSET(LOADER_SectionInfo, 0x08, address);
CHECK_OFFSET(LOADER_SectionInfo, 0x0C, size);
CHECK_OFFSET(LOADER_SectionInfo, 0x0C, name);
CHECK_SIZE(LOADER_SectionInfo, 0x10);

struct LOADER_UserFileInfo
{
   be2_val<uint32_t> size;
   be2_val<uint32_t> magic;
   be2_val<uint32_t> pathStringLength;
   be2_virt_ptr<char> pathString;
   be2_val<uint32_t> fileInfoFlags;
   be2_val<int16_t> tlsModuleIndex;
   be2_val<int16_t> tlsAlignShift;
   be2_val<virt_addr> tlsAddressStart;
   be2_val<uint32_t> tlsSectionSize;
   be2_val<uint32_t> shstrndx;
   be2_val<uint32_t> titleLocation;
   UNKNOWN(0x60 - 0x28);
};
CHECK_OFFSET(LOADER_UserFileInfo, 0x00, size);
CHECK_OFFSET(LOADER_UserFileInfo, 0x04, magic);
CHECK_OFFSET(LOADER_UserFileInfo, 0x08, pathStringLength);
CHECK_OFFSET(LOADER_UserFileInfo, 0x0C, pathString);
CHECK_OFFSET(LOADER_UserFileInfo, 0x10, fileInfoFlags);
CHECK_OFFSET(LOADER_UserFileInfo, 0x14, tlsModuleIndex);
CHECK_OFFSET(LOADER_UserFileInfo, 0x16, tlsAlignShift);
CHECK_OFFSET(LOADER_UserFileInfo, 0x18, tlsAddressStart);
CHECK_OFFSET(LOADER_UserFileInfo, 0x1C, tlsSectionSize);
CHECK_OFFSET(LOADER_UserFileInfo, 0x20, shstrndx);
CHECK_OFFSET(LOADER_UserFileInfo, 0x24, titleLocation);
CHECK_SIZE(LOADER_UserFileInfo, 0x60);

struct LOADER_MinFileInfo
{
   be2_val<uint32_t> size;
   be2_val<uint32_t> version;
   be2_virt_ptr<char> moduleNameBuffer;
   be2_val<uint32_t> moduleNameBufferLen;
   be2_virt_ptr<LOADER_Handle> outKernelHandle;
   be2_virt_ptr<uint32_t> outNumberOfSections;
   be2_virt_ptr<LOADER_SectionInfo> outSectionInfo;
   be2_virt_ptr<uint32_t> outSizeOfFileInfo;
   be2_virt_ptr<LOADER_UserFileInfo> outFileInfo;
   be2_val<uint32_t> dataSize;
   be2_val<uint32_t> dataAlign;
   be2_virt_ptr<void> dataBuffer;
   be2_val<uint32_t> loadSize;
   be2_val<uint32_t> loadAlign;
   be2_virt_ptr<void> loadBuffer;
   be2_val<uint32_t> fileInfoFlags;
   be2_virt_ptr<uint32_t> inoutNextTlsModuleNumber;
   be2_val<uint32_t> pathStringSize;
   be2_virt_ptr<uint32_t> outPathStringSize;
   be2_virt_ptr<char> pathStringBuffer;
   be2_val<uint32_t> fileLocation;
   be2_val<uint32_t> fatalMsgType;
   be2_val<int32_t> fatalErr;
   be2_val<int32_t> error;
   be2_val<uint32_t> fatalLine;
   be2_array<char, 64> fatalFunction;
};
CHECK_OFFSET(LOADER_MinFileInfo, 0x00, size);
CHECK_OFFSET(LOADER_MinFileInfo, 0x04, version);
CHECK_OFFSET(LOADER_MinFileInfo, 0x08, moduleNameBuffer);
CHECK_OFFSET(LOADER_MinFileInfo, 0x0C, moduleNameBufferLen);
CHECK_OFFSET(LOADER_MinFileInfo, 0x10, outKernelHandle);
CHECK_OFFSET(LOADER_MinFileInfo, 0x14, outNumberOfSections);
CHECK_OFFSET(LOADER_MinFileInfo, 0x18, outSectionInfo);
CHECK_OFFSET(LOADER_MinFileInfo, 0x1C, outSizeOfFileInfo);
CHECK_OFFSET(LOADER_MinFileInfo, 0x20, outFileInfo);
CHECK_OFFSET(LOADER_MinFileInfo, 0x24, dataSize);
CHECK_OFFSET(LOADER_MinFileInfo, 0x28, dataAlign);
CHECK_OFFSET(LOADER_MinFileInfo, 0x2C, dataBuffer);
CHECK_OFFSET(LOADER_MinFileInfo, 0x30, loadSize);
CHECK_OFFSET(LOADER_MinFileInfo, 0x34, loadAlign);
CHECK_OFFSET(LOADER_MinFileInfo, 0x38, loadBuffer);
CHECK_OFFSET(LOADER_MinFileInfo, 0x3C, fileInfoFlags);
CHECK_OFFSET(LOADER_MinFileInfo, 0x40, inoutNextTlsModuleNumber);
CHECK_OFFSET(LOADER_MinFileInfo, 0x44, pathStringSize);
CHECK_OFFSET(LOADER_MinFileInfo, 0x48, outPathStringSize);
CHECK_OFFSET(LOADER_MinFileInfo, 0x4C, pathStringBuffer);
CHECK_OFFSET(LOADER_MinFileInfo, 0x50, fileLocation);
CHECK_OFFSET(LOADER_MinFileInfo, 0x54, fatalMsgType);
CHECK_OFFSET(LOADER_MinFileInfo, 0x58, fatalErr);
CHECK_OFFSET(LOADER_MinFileInfo, 0x5C, error);
CHECK_OFFSET(LOADER_MinFileInfo, 0x60, fatalLine);
CHECK_OFFSET(LOADER_MinFileInfo, 0x64, fatalFunction);
CHECK_SIZE(LOADER_MinFileInfo, 0xA4);

namespace internal
{

bool
Loader_ValidateAddrRange(virt_addr addr,
                         uint32_t size);

int32_t
LiValidateAddress(virt_ptr<void> ptr,
                  uint32_t size,
                  uint32_t alignMask,
                  int32_t errorCode,
                  virt_addr minAddr,
                  virt_addr maxAddr,
                  std::string_view name);

int32_t
LiGetMinFileInfo(virt_ptr<LOADED_RPL> rpl,
                 virt_ptr<LOADER_MinFileInfo> info);

int32_t
LiValidateMinFileInfo(virt_ptr<LOADER_MinFileInfo> minFileInfo,
                      std::string_view funcName);

void
updateFileInfoForUser(virt_ptr<LOADED_RPL> rpl,
                      virt_ptr<LOADER_UserFileInfo> userFileInfo,
                      virt_ptr<LOADER_MinFileInfo> minFileInfo);

} // namespace internal

} // namespace cafe::loader
