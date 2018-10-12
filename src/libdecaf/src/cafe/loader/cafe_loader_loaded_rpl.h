#pragma once
#include "cafe_loader_rpl.h"
#include "cafe/kernel/cafe_kernel_processid.h"
#include "ios/mcp/ios_mcp_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::loader
{

enum LoadStateFlags : uint32_t
{
   LoaderPrep = 0,
   LoaderSetup = 1 << 0,
   LoaderStateFlag2 = 1 << 1,
   LoaderStateFlag4 = 1 << 2,

   //! Likely means this is a circular dependency
   LoaderStateFlag8 = 1 << 3,

   //! Likely means linked
   LoaderStateFlags_Unk0x20000000 = 0x20000000,
};

struct LOADER_UserFileInfo;

struct LOADED_RPL
{
   be2_virt_ptr<void> globals;
   be2_val<uint32_t> selfBufferSize;
   be2_virt_ptr<char> moduleNameBuffer;
   be2_val<uint32_t> moduleNameLen;
   be2_val<uint32_t> moduleNameBufferSize;
   be2_virt_ptr<void> pathBuffer;
   be2_val<uint32_t> pathBufferSize;
   be2_struct<rpl::Header> elfHeader;
   be2_virt_ptr<void> sectionHeaderBuffer;
   be2_val<uint32_t> sectionHeaderBufferSize;
   be2_virt_ptr<rpl::RPLFileInfo_v4_2> fileInfoBuffer;
   be2_val<uint32_t> fileInfoSize;
   be2_val<uint32_t> fileInfoBufferSize;
   be2_virt_ptr<void> crcBuffer;
   be2_val<uint32_t> crcBufferSize;
   be2_val<uint32_t> lastSectionCrc;
   be2_val<LoadStateFlags> loadStateFlags;
   be2_val<virt_addr> entryPoint;
   be2_val<uint32_t> upcomingBufferNumber;
   be2_virt_ptr<void> lastChunkBuffer;
   be2_val<uint32_t> virtualFileBaseOffset;
   be2_val<uint32_t> fileOffset;
   be2_val<uint32_t> upcomingFileOffset;
   be2_val<ios::mcp::MCPFileType> fileType;
   be2_val<uint32_t> totalBytesRead;
   be2_virt_ptr<void> chunkBuffer;
   UNKNOWN(0xAC - 0x98);
   be2_val<cafe::kernel::UniqueProcessId> upid;
   be2_virt_ptr<void> virtualFileBase;
   be2_virt_ptr<void> textBuffer;
   be2_val<uint32_t> textBufferSize;
   be2_virt_ptr<void> dataBuffer;
   be2_virt_ptr<void> loadBuffer;
   be2_virt_ptr<void> compressedRelocationsBuffer;
   be2_val<uint32_t> compressedRelocationsBufferSize;
   be2_val<virt_addr> postTrampBuffer;
   be2_val<virt_addr> textAddr;
   be2_val<uint32_t> textOffset;
   be2_val<uint32_t> textSize;
   be2_val<virt_addr> dataAddr;
   be2_val<uint32_t> dataOffset;
   be2_val<uint32_t> dataSize;
   be2_val<virt_addr> loadAddr;
   be2_val<uint32_t> loadOffset;
   be2_val<uint32_t> loadSize;
   be2_virt_ptr<virt_addr> sectionAddressBuffer;
   be2_val<uint32_t> sectionAddressBufferSize;
   be2_val<uint32_t> numFuncExports;
   be2_virt_ptr<void> funcExports;
   be2_val<uint32_t> numDataExports;
   be2_virt_ptr<void> dataExports;

   //! Pointer to last struct returned from LOADER_GetFileInfo.
   be2_virt_ptr<LOADER_UserFileInfo> userFileInfo;

   //! Size of the file info structure returned from LOADER_GetFileInfo.
   be2_val<uint32_t> userFileInfoSize;

   be2_virt_ptr<LOADED_RPL> nextLoadedRpl;
};
CHECK_OFFSET(LOADED_RPL, 0x00, globals);
CHECK_OFFSET(LOADED_RPL, 0x04, selfBufferSize);
CHECK_OFFSET(LOADED_RPL, 0x08, moduleNameBuffer);
CHECK_OFFSET(LOADED_RPL, 0x0C, moduleNameLen);
CHECK_OFFSET(LOADED_RPL, 0x10, moduleNameBufferSize);
CHECK_OFFSET(LOADED_RPL, 0x14, pathBuffer);
CHECK_OFFSET(LOADED_RPL, 0x18, pathBufferSize);
CHECK_OFFSET(LOADED_RPL, 0x1C, elfHeader);
CHECK_OFFSET(LOADED_RPL, 0x50, sectionHeaderBuffer);
CHECK_OFFSET(LOADED_RPL, 0x54, sectionHeaderBufferSize);
CHECK_OFFSET(LOADED_RPL, 0x58, fileInfoBuffer);
CHECK_OFFSET(LOADED_RPL, 0x5C, fileInfoSize);
CHECK_OFFSET(LOADED_RPL, 0x60, fileInfoBufferSize);
CHECK_OFFSET(LOADED_RPL, 0x64, crcBuffer);
CHECK_OFFSET(LOADED_RPL, 0x68, crcBufferSize);
CHECK_OFFSET(LOADED_RPL, 0x6C, lastSectionCrc);
CHECK_OFFSET(LOADED_RPL, 0x70, loadStateFlags);
CHECK_OFFSET(LOADED_RPL, 0x74, entryPoint);
CHECK_OFFSET(LOADED_RPL, 0x78, upcomingBufferNumber);
CHECK_OFFSET(LOADED_RPL, 0x7C, lastChunkBuffer);
CHECK_OFFSET(LOADED_RPL, 0x80, virtualFileBaseOffset);
CHECK_OFFSET(LOADED_RPL, 0x84, fileOffset);
CHECK_OFFSET(LOADED_RPL, 0x88, upcomingFileOffset);
CHECK_OFFSET(LOADED_RPL, 0x8C, fileType);
CHECK_OFFSET(LOADED_RPL, 0x90, totalBytesRead);
CHECK_OFFSET(LOADED_RPL, 0x94, chunkBuffer);
CHECK_OFFSET(LOADED_RPL, 0xAC, upid);
CHECK_OFFSET(LOADED_RPL, 0xB0, virtualFileBase);
CHECK_OFFSET(LOADED_RPL, 0xB4, textBuffer);
CHECK_OFFSET(LOADED_RPL, 0xB8, textBufferSize);
CHECK_OFFSET(LOADED_RPL, 0xBC, dataBuffer);
CHECK_OFFSET(LOADED_RPL, 0xC0, loadBuffer);
CHECK_OFFSET(LOADED_RPL, 0xC4, compressedRelocationsBuffer);
CHECK_OFFSET(LOADED_RPL, 0xC8, compressedRelocationsBufferSize);
CHECK_OFFSET(LOADED_RPL, 0xCC, postTrampBuffer);
CHECK_OFFSET(LOADED_RPL, 0xD0, textAddr);
CHECK_OFFSET(LOADED_RPL, 0xD4, textOffset);
CHECK_OFFSET(LOADED_RPL, 0xD8, textSize);
CHECK_OFFSET(LOADED_RPL, 0xDC, dataAddr);
CHECK_OFFSET(LOADED_RPL, 0xE0, dataOffset);
CHECK_OFFSET(LOADED_RPL, 0xE4, dataSize);
CHECK_OFFSET(LOADED_RPL, 0xE8, loadAddr);
CHECK_OFFSET(LOADED_RPL, 0xEC, loadOffset);
CHECK_OFFSET(LOADED_RPL, 0xF0, loadSize);
CHECK_OFFSET(LOADED_RPL, 0xF4, sectionAddressBuffer);
CHECK_OFFSET(LOADED_RPL, 0xF8, sectionAddressBufferSize);
CHECK_OFFSET(LOADED_RPL, 0xFC, numFuncExports);
CHECK_OFFSET(LOADED_RPL, 0x100, funcExports);
CHECK_OFFSET(LOADED_RPL, 0x104, numDataExports);
CHECK_OFFSET(LOADED_RPL, 0x108, dataExports);
CHECK_OFFSET(LOADED_RPL, 0x10C, userFileInfo);
CHECK_OFFSET(LOADED_RPL, 0x110, userFileInfoSize);
CHECK_OFFSET(LOADED_RPL, 0x114, nextLoadedRpl);
CHECK_SIZE(LOADED_RPL, 0x118);

} // namespace cafe::loader
