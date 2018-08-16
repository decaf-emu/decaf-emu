#pragma once
#include "cafe/kernel/cafe_kernel_processid.h"
#include <libcpu/be2_struct.h>

namespace cafe::loader
{

struct LOADER_MinFileInfo;
struct LOADED_RPL;

namespace internal
{

struct LiBasicsLoadArgs;

int32_t
LiLoadForPrep(virt_ptr<char> moduleName,
              uint32_t moduleNameLen,
              virt_ptr<void> chunkBuffer,
              virt_ptr<LOADED_RPL> *outLoadedRpl,
              LiBasicsLoadArgs *loadArgs,
              uint32_t unk);

int32_t
LOADER_Prep(kernel::UniqueProcessId upid,
            virt_ptr<LOADER_MinFileInfo> minFileInfo);

} // namespace internal

} // namespace cafe::loader
