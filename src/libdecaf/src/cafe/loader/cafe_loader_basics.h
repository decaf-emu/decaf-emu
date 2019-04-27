#pragma once
#include "cafe/cafe_tinyheap.h"
#include "cafe/kernel/cafe_kernel_processid.h"
#include "ios/mcp/ios_mcp_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::loader
{

struct LOADED_RPL;

namespace internal
{

struct LiBasicsLoadArgs
{
   be2_val<cafe::kernel::UniqueProcessId> upid;
   be2_virt_ptr<LOADED_RPL> loadedRpl;
   be2_virt_ptr<TinyHeap> readHeapTracking;
   be2_val<uint32_t> pathNameLen;
   be2_virt_ptr<char> pathName;
   UNKNOWN(0x4);
   be2_val<ios::mcp::MCPFileType> fileType;
   be2_virt_ptr<void> chunkBuffer;
   be2_val<uint32_t> chunkBufferSize;
   be2_val<uint32_t> fileOffset;
};
CHECK_OFFSET(LiBasicsLoadArgs, 0x00, upid);
CHECK_OFFSET(LiBasicsLoadArgs, 0x04, loadedRpl);
CHECK_OFFSET(LiBasicsLoadArgs, 0x08, readHeapTracking);
CHECK_OFFSET(LiBasicsLoadArgs, 0x0C, pathNameLen);
CHECK_OFFSET(LiBasicsLoadArgs, 0x10, pathName);
CHECK_OFFSET(LiBasicsLoadArgs, 0x18, fileType);
CHECK_OFFSET(LiBasicsLoadArgs, 0x1C, chunkBuffer);
CHECK_OFFSET(LiBasicsLoadArgs, 0x20, chunkBufferSize);
CHECK_OFFSET(LiBasicsLoadArgs, 0x24, fileOffset);
CHECK_SIZE(LiBasicsLoadArgs, 0x28);

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
                uint32_t arg_C);

} // namespace internal

} // namespace cafe::loader
