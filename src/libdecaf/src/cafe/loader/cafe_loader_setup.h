#pragma once
#include "cafe_loader_loaded_rpl.h"
#include "cafe_loader_minfileinfo.h"
#include "cafe/kernel/cafe_kernel_processid.h"

#include <cstdint>
#include <libcpu/be2_struct.h>

namespace cafe::loader::internal
{

int32_t
LiSetupOneRPL(kernel::UniqueProcessId upid,
              virt_ptr<LOADED_RPL> rpl,
              virt_ptr<TinyHeap> codeHeapTracking,
              virt_ptr<TinyHeap> dataHeapTracking);

int32_t
LOADER_Setup(kernel::UniqueProcessId upid,
             LOADER_Handle handle,
             BOOL isPurge,
             virt_ptr<LOADER_MinFileInfo> minFileInfo);

} // namespace cafe::loader::internal
