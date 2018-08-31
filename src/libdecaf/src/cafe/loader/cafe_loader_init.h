#pragma once
#include "cafe/kernel/cafe_kernel_processid.h"
#include <libcpu/be2_struct.h>

namespace cafe::loader
{

struct LOADED_RPL;

struct RPL_STARTINFO
{
   be2_val<virt_addr> entryPoint;
   be2_val<virt_addr> dataAreaStart;
   be2_val<virt_addr> sdaBase;
   be2_val<virt_addr> sda2Base;
   be2_val<uint32_t> unk0x10;
   be2_val<uint32_t> stackSize;
   be2_val<uint32_t> systemHeapSize;
   be2_val<uint32_t> appFlags;
   be2_virt_ptr<LOADED_RPL> coreinit;
   be2_val<virt_addr> dataAreaEnd;
};
CHECK_OFFSET(RPL_STARTINFO, 0x00, entryPoint);
CHECK_OFFSET(RPL_STARTINFO, 0x04, dataAreaStart);
CHECK_OFFSET(RPL_STARTINFO, 0x08, sdaBase);
CHECK_OFFSET(RPL_STARTINFO, 0x0C, sda2Base);
CHECK_OFFSET(RPL_STARTINFO, 0x10, unk0x10);
CHECK_OFFSET(RPL_STARTINFO, 0x14, stackSize);
CHECK_OFFSET(RPL_STARTINFO, 0x18, systemHeapSize);
CHECK_OFFSET(RPL_STARTINFO, 0x1C, appFlags);
CHECK_OFFSET(RPL_STARTINFO, 0x20, coreinit);
CHECK_OFFSET(RPL_STARTINFO, 0x24, dataAreaEnd);
CHECK_SIZE(RPL_STARTINFO, 0x28);

namespace internal
{

int32_t
LOADER_Init(kernel::UniqueProcessId upid,
            uint32_t numCodeAreaHeapBlocks,
            uint32_t maxCodeSize,
            uint32_t maxDataSize,
            virt_ptr<virt_ptr<LOADED_RPL>> outLoadedRpx,
            virt_ptr<virt_ptr<LOADED_RPL>> outModuleList,
            virt_ptr<RPL_STARTINFO> startInfo);

} // namespace internal

} // namespace cafe::loader
