#pragma once
#include "cafe/loader/cafe_loader_basics.h"
#include "cafe/kernel/cafe_kernel_context.h"
#include "cafe/loader/cafe_loader_entry.h"
#include "cafe/loader/cafe_loader_init.h"
#include "cafe/kernel/cafe_kernel_processid.h"

#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::loader
{

// 0xEFE00000 - 0xEFE01000 = Root RPX Name
struct RootRPX
{
   be2_array<char, 0x1000> name;
};
CHECK_OFFSET(RootRPX, 0x00, name);
CHECK_SIZE(RootRPX, 0x1000);

// 0xEFE01000 - 0xEFE02000 = Loader global variables
struct GlobalStorage
{
   be2_val<kernel::UniqueProcessId> currentUpid;
   be2_virt_ptr<TinyHeap> processCodeHeap;
   be2_val<uint32_t> processCodeHeapTrackingBlockSize;
   be2_val<uint32_t> numCodeAreaHeapBlocks;
   be2_val<uint32_t> availableCodeSize;
   be2_val<uint32_t> maxCodeSize;
   be2_val<uint32_t> maxDataSize;
   be2_val<uint32_t> sdaBase;
   be2_val<uint32_t> sda2Base;
   be2_val<BOOL> userHasControl;
   be2_virt_ptr<LOADED_RPL> loadedRpx;
   be2_virt_ptr<LOADED_RPL> firstLoadedRpl;
   be2_virt_ptr<LOADED_RPL> lastLoadedRpl;
   UNKNOWN(0xC);
};
CHECK_OFFSET(GlobalStorage, 0x00, currentUpid);
CHECK_OFFSET(GlobalStorage, 0x04, processCodeHeap);
CHECK_OFFSET(GlobalStorage, 0x08, processCodeHeapTrackingBlockSize);
CHECK_OFFSET(GlobalStorage, 0x0C, numCodeAreaHeapBlocks);
CHECK_OFFSET(GlobalStorage, 0x10, availableCodeSize);
CHECK_OFFSET(GlobalStorage, 0x14, maxCodeSize);
CHECK_OFFSET(GlobalStorage, 0x18, maxDataSize);
CHECK_OFFSET(GlobalStorage, 0x1C, sdaBase);
CHECK_OFFSET(GlobalStorage, 0x20, sda2Base);
CHECK_OFFSET(GlobalStorage, 0x24, userHasControl);
CHECK_OFFSET(GlobalStorage, 0x28, loadedRpx);
CHECK_OFFSET(GlobalStorage, 0x2C, firstLoadedRpl);
CHECK_OFFSET(GlobalStorage, 0x30, lastLoadedRpl);
CHECK_SIZE(GlobalStorage, 0x40);

// 0xEFE02000 - 0xEFE0A400 = Loader context & stack
struct ContextStorage
{
   be2_array<uint8_t, 0x2400> stack0;
   be2_array<uint8_t, 0x2400> stack1;
   be2_array<uint8_t, 0x2400> stack2;
   be2_struct<kernel::Context> context0;
   UNKNOWN(0x4E0);
   be2_struct<kernel::Context> context1;
   UNKNOWN(0x4E0);
   be2_struct<kernel::Context> context2;
   UNKNOWN(0x4E0);
};
CHECK_OFFSET(ContextStorage, 0x0000, stack0);
CHECK_OFFSET(ContextStorage, 0x2400, stack1);
CHECK_OFFSET(ContextStorage, 0x4800, stack2);
CHECK_OFFSET(ContextStorage, 0x6C00, context0);
CHECK_OFFSET(ContextStorage, 0x7400, context1);
CHECK_OFFSET(ContextStorage, 0x7C00, context2);
CHECK_SIZE(ContextStorage, 0x8400);

// 0xEFE0A400 - 0xEFE0A4C0 = SharedData
struct KernelIpcStorage
{
   //! Set to 0 and never read?
   be2_val<uint32_t> unk0x00;
   be2_val<kernel::ProcessFlags> processFlags;
   be2_val<kernel::UniqueProcessId> callerProcessId;
   be2_val<kernel::UniqueProcessId> targetProcessId;
   be2_val<uint32_t> numCodeAreaHeapBlocks;
   be2_val<uint32_t> maxCodeSize;
   be2_val<uint32_t> maxDataSize;
   be2_val<uint32_t> procTitleLoc;
   be2_virt_ptr<loader::LOADED_RPL> rpxModule;
   be2_virt_ptr<loader::LOADED_RPL> loadedModuleList;
   be2_val<uint32_t> unk0x28;
   be2_val<uint32_t> fatalMsgType;
   be2_val<int32_t> fatalErr;
   //! Error returned from LOADER_Init called from loader start
   be2_val<int32_t> loaderInitError;
   be2_val<uint32_t> fatalLine;
   be2_array<char, 0x40> fatalFunction;
   be2_struct<loader::RPL_STARTINFO> startInfo;
   be2_struct<loader::LOADER_EntryParams> entryParams;
};
CHECK_OFFSET(KernelIpcStorage, 0x00, unk0x00);
CHECK_OFFSET(KernelIpcStorage, 0x04, processFlags);
CHECK_OFFSET(KernelIpcStorage, 0x08, callerProcessId);
CHECK_OFFSET(KernelIpcStorage, 0x0C, targetProcessId);
CHECK_OFFSET(KernelIpcStorage, 0x10, numCodeAreaHeapBlocks);
CHECK_OFFSET(KernelIpcStorage, 0x14, maxCodeSize);
CHECK_OFFSET(KernelIpcStorage, 0x18, maxDataSize);
CHECK_OFFSET(KernelIpcStorage, 0x1C, procTitleLoc);
CHECK_OFFSET(KernelIpcStorage, 0x20, rpxModule);
CHECK_OFFSET(KernelIpcStorage, 0x24, loadedModuleList);
CHECK_OFFSET(KernelIpcStorage, 0x28, unk0x28);
CHECK_OFFSET(KernelIpcStorage, 0x2C, fatalMsgType);
CHECK_OFFSET(KernelIpcStorage, 0x30, fatalErr);
CHECK_OFFSET(KernelIpcStorage, 0x34, loaderInitError);
CHECK_OFFSET(KernelIpcStorage, 0x38, fatalLine);
CHECK_OFFSET(KernelIpcStorage, 0x3C, fatalFunction);
CHECK_OFFSET(KernelIpcStorage, 0x7C, startInfo);
CHECK_OFFSET(KernelIpcStorage, 0xA4, entryParams);
CHECK_SIZE(KernelIpcStorage, 0xDC);

// 0xEFE0B000 - 0xEFE80000 = loader .rodata & .data & .bss

void
setLoadRpxName(std::string_view name);

virt_ptr<char>
getLoadRpxName();

virt_ptr<GlobalStorage>
getGlobalStorage();

virt_ptr<ContextStorage>
getContextStorage();

virt_ptr<KernelIpcStorage>
getKernelIpcStorage();

} // namespace cafe::loader
