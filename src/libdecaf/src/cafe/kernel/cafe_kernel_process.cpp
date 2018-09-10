#include "cafe_kernel.h"
#include "cafe_kernel_loader.h"
#include "cafe_kernel_mmu.h"
#include "cafe_kernel_process.h"

#include "ios/mcp/ios_mcp_mcp_types.h"
#include "cafe/libraries/cafe_hle.h"
#include "cafe/loader/cafe_loader_globals.h"
#include "cafe/loader/cafe_loader_loaded_rpl.h"

#include <array>
#include <libcpu/cpu_config.h>

namespace cafe::kernel::internal
{

constexpr auto MinCodeSize = 0x20000u;
constexpr auto MaxCodeSize = 0xE000000u;
constexpr auto MinDataSize = 0x700000u;
constexpr auto UnkReserveSize = 0x60000u;

struct CoreProcessData
{
   RamPartitionId rampid;
   UniqueProcessId upid;
   KernelProcessId pid;
};

static std::array<CoreProcessData, 3>
sCoreProcessData;

static std::array<RamPartitionData, NumRamPartitions>
sRamPartitionData;

struct RamPartitionConfig
{
   RamPartitionId id;
   phys_addr base;
   uint32_t size;
};

static constexpr RamPartitionConfig DevRamPartitionConfig[] = {
   { RamPartitionId::Kernel,           phys_addr { 0 },                    0 },
   { RamPartitionId::OverlayMenu,      phys_addr { 0x28000000 },   0x8000000 },
   { RamPartitionId::Root,             phys_addr { 0x30000000 },   0x2000000 },
   { RamPartitionId::ErrorDisplay,     phys_addr { 0x33000000 },   0x1000000 },
   { RamPartitionId::OverlayApp,       phys_addr { 0x34000000 },  0x1C000000 },
   { RamPartitionId::MainApplication,  phys_addr { 0x50000000 },  0x80000000 },
};

static constexpr RamPartitionConfig RetailRamPartitionConfig[] = {
   { RamPartitionId::Kernel,           phys_addr { 0 },                    0 },
   { RamPartitionId::OverlayMenu,      phys_addr { 0x28000000 },   0x8000000 },
   { RamPartitionId::Root,             phys_addr { 0x30000000 },   0x2000000 },
   { RamPartitionId::ErrorDisplay,     phys_addr { 0x33000000 },   0x1000000 },
   { RamPartitionId::OverlayApp,       phys_addr { 0x34000000 },  0x1C000000 },
   { RamPartitionId::MainApplication,  phys_addr { 0x50000000 },  0x40000000 },
};

static const RamPartitionConfig *
sRamPartitionConfig = RetailRamPartitionConfig;

static void
allocateRamPartition(RamPartitionId rampid,
                     uint32_t max_size,
                     uint32_t avail_size,
                     uint32_t codegen_size,
                     uint32_t max_codesize,
                     uint32_t codegen_core,
                     RamPartitionAllocation *info)
{
   const RamPartitionConfig *config = nullptr;
   for (auto i = 0u; i < NumRamPartitions; ++i) {
      if (sRamPartitionConfig[i].id == rampid) {
         config = &sRamPartitionConfig[i];
         break;
      }
   }

   decaf_check(config);

   max_codesize = std::max<uint32_t>(max_codesize, MinCodeSize);
   max_codesize = std::min<uint32_t>(max_codesize, MaxCodeSize);
   max_codesize = align_up(max_codesize, PageSize);
   avail_size = align_up(avail_size, PageSize);
   codegen_size = align_up(codegen_size, PageSize);

   decaf_check(max_codesize < max_size);
   decaf_check(max_codesize <= MaxCodeSize);

   info->dataStart = config->base;
   info->codeEnd = config->base + config->size;
   info->codeStart = align_down(info->codeEnd - max_codesize, PageSize);
   info->codeGenStart = align_down(info->codeStart - codegen_size, PageSize);
   info->availStart = align_down(info->codeGenStart - UnkReserveSize - avail_size, PageSize);
   info->codegen_core = codegen_core;
   info->unk0x18 = 0;
   info->unk0x1C = 0;
   decaf_check(info->availStart - info->dataStart >= MinDataSize);
}

RamPartitionData *
getCurrentRamPartitionData()
{
   return getRamPartitionData(getCurrentRamPartitionId());
}

RamPartitionId
getCurrentRamPartitionId()
{
   return sCoreProcessData[cpu::this_core::id()].rampid;
}

KernelProcessId
getCurrentKernelProcessId()
{
   return getCurrentRamPartitionData()->coreKernelProcessId[cpu::this_core::id()];
}

UniqueProcessId
getCurrentUniqueProcessId()
{
   return getCurrentRamPartitionData()->uniqueProcessId;
}

ios::mcp::MCPTitleId
getCurrentTitleId()
{
   return getCurrentRamPartitionData()->titleInfo.titleId;
}

RamPartitionData *
getRamPartitionData(RamPartitionId id)
{
   return &sRamPartitionData[static_cast<size_t>(id)];
}

void
setCoreToProcessId(RamPartitionId ramPartitionId,
                   KernelProcessId kernelProcessId)
{
   auto coreId = cpu::this_core::id();
   auto partitionData = getRamPartitionData(ramPartitionId);

   // Check if we need to set a new ram ramPartitionId
   if (sCoreProcessData[coreId].rampid != ramPartitionId) {
      if (coreId == 1) {
         // TODO: When we have per-core address space then do not check coreId
         loadAddressSpace(&partitionData->addressSpace);
      }

      setActiveAddressSpace(&partitionData->addressSpace);
      sCoreProcessData[coreId].rampid = ramPartitionId;
   }

   partitionData->coreKernelProcessId[coreId] = kernelProcessId;
}

static void
initialisePerCoreStartInfo(ProcessPerCoreStartInfo *core0,
                           ProcessPerCoreStartInfo *core1,
                           ProcessPerCoreStartInfo *core2)
{
   auto partitionData = getCurrentRamPartitionData();
   auto stackSize0 = std::max<uint32_t>(0x4000u, partitionData->startInfo.stackSize);
   auto stackSize1 = std::max<uint32_t>(0x4000u, partitionData->startInfo.stackSize);
   auto stackSize2 = std::max<uint32_t>(0x4000u, partitionData->startInfo.stackSize);
   auto exceptionStackSize0 = 0x1000u;
   auto exceptionStackSize1 = 0x1000u;
   auto exceptionStackSize2 = 0x1000u;

   core0->entryPoint = partitionData->startInfo.entryPoint;
   core1->entryPoint = partitionData->startInfo.entryPoint;
   core2->entryPoint = partitionData->startInfo.entryPoint;
   core0->sdaBase = partitionData->startInfo.sdaBase;
   core1->sdaBase = partitionData->startInfo.sdaBase;
   core2->sdaBase = partitionData->startInfo.sdaBase;
   core0->sda2Base = partitionData->startInfo.sda2Base;
   core1->sda2Base = partitionData->startInfo.sda2Base;
   core2->sda2Base = partitionData->startInfo.sda2Base;
   core0->unk0x1C = partitionData->startInfo.unk0x10;
   core1->unk0x1C = partitionData->startInfo.unk0x10;
   core2->unk0x1C = partitionData->startInfo.unk0x10;

   core0->stackEnd = align_up(partitionData->startInfo.dataAreaStart, 8);
   core1->stackEnd = align_up(core0->stackEnd + stackSize0, 8);
   core2->stackEnd = align_up(core1->stackEnd + stackSize1, 8);

   core0->stackBase = align_down(core0->stackEnd + stackSize0, 8) - 8;
   core1->stackBase = align_down(core1->stackEnd + stackSize1, 8) - 8;
   core2->stackBase = align_down(core2->stackEnd + stackSize2, 8) - 8;

   core0->exceptionStackEnd = align_up(core2->stackEnd + stackSize2, 8);
   core1->exceptionStackEnd = align_up(core0->exceptionStackEnd + exceptionStackSize0, 8);
   core2->exceptionStackEnd = align_up(core1->exceptionStackEnd + exceptionStackSize1, 8);

   core0->exceptionStackBase = align_down(core0->exceptionStackEnd + exceptionStackSize0, 8) - 8;
   core1->exceptionStackBase = align_down(core1->exceptionStackEnd + exceptionStackSize1, 8) - 8;
   core2->exceptionStackBase = align_down(core2->exceptionStackEnd + exceptionStackSize2, 8) - 8;

   auto stacksEnd = align_up(core2->exceptionStackEnd + exceptionStackSize2, 8);
   decaf_check(stacksEnd > partitionData->startInfo.dataAreaStart &&
               stacksEnd <= partitionData->startInfo.dataAreaEnd);

   partitionData->startInfo.dataAreaStart = stacksEnd;
}

loader::RPL_STARTINFO *
getCurrentRamPartitionStartInfo()
{
   return &getCurrentRamPartitionData()->startInfo;
}

void
loadGameProcess(std::string_view rpx,
                ios::mcp::MCPPPrepareTitleInfo &titleInfo)
{
   auto rampid = RamPartitionId::MainApplication;
   auto partitionData = getRamPartitionData(rampid);
   partitionData->argstr = reinterpret_cast<char *>(std::addressof(titleInfo.argstr.at(0)));
   partitionData->uniqueProcessId = UniqueProcessId::Game;
   partitionData->titleId = titleInfo.titleId;

   // Initialise memory
   allocateRamPartition(rampid,
                        titleInfo.max_size,
                        titleInfo.avail_size,
                        titleInfo.codegen_size,
                        titleInfo.max_codesize,
                        titleInfo.codegen_core,
                        &partitionData->ramPartitionAllocation);

   partitionData->overlayArenaEnabled = false;
   if (rampid == RamPartitionId::MainApplication) {
      partitionData->overlayArenaEnabled = !!titleInfo.overlay_arena;
   }

   initialiseAddressSpace(
      &partitionData->addressSpace,
      rampid,
      partitionData->ramPartitionAllocation.codeStart,
      static_cast<uint32_t>(partitionData->ramPartitionAllocation.codeEnd - partitionData->ramPartitionAllocation.codeStart),
      partitionData->ramPartitionAllocation.dataStart,
      static_cast<uint32_t>(partitionData->ramPartitionAllocation.availStart - partitionData->ramPartitionAllocation.dataStart),
      0, 0,
      partitionData->ramPartitionAllocation.availStart,
      static_cast<uint32_t>(partitionData->ramPartitionAllocation.codeGenStart - partitionData->ramPartitionAllocation.availStart),
      partitionData->ramPartitionAllocation.codeGenStart,
      static_cast<uint32_t>(partitionData->ramPartitionAllocation.codeStart - partitionData->ramPartitionAllocation.codeGenStart),
      partitionData->ramPartitionAllocation.codegen_core,
      partitionData->overlayArenaEnabled);

   setCoreToProcessId(rampid,
                      KernelProcessId::Loader);

   initialiseCoreProcess(cpu::this_core::id(),
                         rampid,
                         UniqueProcessId::Game,
                         KernelProcessId::Loader);

   auto num_codearea_heap_blocks = titleInfo.num_codearea_heap_blocks;
   if (!num_codearea_heap_blocks) {
      num_codearea_heap_blocks = 256u;
   }

   auto num_workarea_heap_blocks = titleInfo.num_workarea_heap_blocks;
   if (!num_workarea_heap_blocks) {
      num_workarea_heap_blocks = 512u;
   }

   // Run the loader
   cafe::loader::setLoadRpxName(rpx);
   KiRPLStartup(
      cafe::kernel::UniqueProcessId::Kernel,
      cafe::kernel::UniqueProcessId::Game,
      cafe::kernel::ProcessFlags::get(0).debugLevel(cafe::kernel::DebugLevel::Verbose),
      num_codearea_heap_blocks + num_workarea_heap_blocks,
      static_cast<uint32_t>(partitionData->ramPartitionAllocation.codeEnd - partitionData->ramPartitionAllocation.codeStart),
      static_cast<uint32_t>(partitionData->ramPartitionAllocation.availStart - partitionData->ramPartitionAllocation.dataStart),
      0);

   // Notify jit of read only sections in the RPX
   if (cpu::config::jit::rodata_read_only) {
      auto rpx = cafe::loader::getGlobalStorage()->loadedRpx;
      auto shStrSection = virt_ptr<char> { nullptr };
      if (auto shstrndx = rpx->elfHeader.shstrndx) {
         shStrSection = virt_cast<char *>(rpx->sectionAddressBuffer[shstrndx]);
      }

      for (auto i = 0u; i < rpx->elfHeader.shnum; ++i) {
         auto sectionHeader =
            virt_cast<loader::rpl::SectionHeader *>(
               virt_cast<virt_addr>(rpx->sectionHeaderBuffer) +
               (i * rpx->elfHeader.shentsize));
         auto sectionAddress = rpx->sectionAddressBuffer[i];
         if (!sectionAddress ||
             sectionHeader->type != loader::rpl::SHT_PROGBITS) {
            continue;
         }

         if (shStrSection && sectionHeader->name) {
            auto name = shStrSection + sectionHeader->name;
            if (strcmp(name.getRawPointer(), ".rodata") == 0) {
               cpu::addJitReadOnlyRange(sectionAddress,
                                        sectionHeader->size);
               continue;
            }
         }

         if (!(sectionHeader->flags & loader::rpl::SHF_WRITE)) {
            // TODO: Fix me
            // When we have a small section, e.g. .syscall section with
            // sectionHeader->size == 8, we seem to break binrec
            //cpu::addJitReadOnlyRange(sectionAddress,
            //                         sectionHeader->size);
         }
      }
   }

   // Run the HLE relocation for coreinit.
   auto &startInfo = cafe::loader::getKernelIpcStorage()->startInfo;
   auto coreinitRpl = startInfo.coreinit;
   cafe::hle::relocateLibrary(
      std::string_view { coreinitRpl->moduleNameBuffer.getRawPointer(), coreinitRpl->moduleNameLen },
      virt_cast<virt_addr>(coreinitRpl->textBuffer),
      virt_cast<virt_addr>(coreinitRpl->dataBuffer)
   );
}


/**
 * KiProcess_FinishInitAndPreload
 */
void
finishInitAndPreload()
{
   auto partitionData = getCurrentRamPartitionData();
   auto loaderIpcStorage = cafe::loader::getKernelIpcStorage();
   partitionData->startInfo = loaderIpcStorage->startInfo;
   partitionData->loadedRpx = loaderIpcStorage->rpxModule;
   partitionData->loadedModuleList = loaderIpcStorage->loadedModuleList;

   initialisePerCoreStartInfo(&partitionData->perCoreStartInfo[0],
                              &partitionData->perCoreStartInfo[1],
                              &partitionData->perCoreStartInfo[2]);

   // KiProcess_Launch
   using EntryPointFn = virt_func_ptr<void()>;
   auto entryPoint = partitionData->perCoreStartInfo[cpu::this_core::id()].entryPoint;
   cafe::invoke(cpu::this_core::state(),
                virt_func_cast<EntryPointFn>(entryPoint));
}

void
initialiseCoreProcess(int coreId,
                      RamPartitionId rampid,
                      UniqueProcessId upid,
                      KernelProcessId pid)
{
   auto &data = sCoreProcessData[coreId];
   data.rampid = rampid;
   data.upid = upid;
   data.pid = pid;
}

void
initialiseProcessData()
{
   for (auto i = 0u; i < sRamPartitionData.size(); ++i) {
      auto &data = sRamPartitionData[i];
      // data.state = 0
      // data.field_0 = -1
      data.ramPartitionId = static_cast<RamPartitionId>(i);
      data.coreKernelProcessId[0] = KernelProcessId::Invalid;
      data.coreKernelProcessId[1] = KernelProcessId::Invalid;
      data.coreKernelProcessId[2] = KernelProcessId::Invalid;
   }
}

} // namespace cafe::kernel::internal


namespace cafe::kernel
{

void
exitProcess(int code)
{
   auto partitionData = internal::getCurrentRamPartitionData();
   partitionData->exitCode = code;
   internal::exit();
}

int
getProcessExitCode(RamPartitionId rampid)
{
   return internal::getRamPartitionData(rampid)->exitCode;
}

} // namespace cafe::kernel