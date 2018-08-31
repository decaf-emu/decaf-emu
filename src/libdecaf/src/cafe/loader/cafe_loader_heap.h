#pragma once
#include "cafe/cafe_tinyheap.h"
#include "ios/mcp/ios_mcp_enum.h"

#include <cstdint>
#include <libcpu/be2_struct.h>

namespace cafe::loader::internal
{

int32_t
LiCacheLineCorrectAllocEx(virt_ptr<TinyHeap> heap,
                          uint32_t textSize,
                          int32_t textAlign,
                          virt_ptr<void> *outPtr,
                          uint32_t unk,
                          uint32_t *outAllocSize,
                          uint32_t *outLargestFree,
                          ios::mcp::MCPFileType fileType);

void
LiCacheLineCorrectFreeEx(virt_ptr<TinyHeap> heap,
                         virt_ptr<void> ptr,
                         uint32_t size);

} // namespace cafe::loader::internal
