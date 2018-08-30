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

namespace cafe::kernel
{

constexpr auto NumRamPartitions = 8u;
constexpr auto MinCodeSize = 0x20000u;
constexpr auto MaxCodeSize = 0xE000000u;
constexpr auto MinDataSize = 0x700000u;
constexpr auto UnkReserveSize = 0x60000u;

static std::array<ProcessData, NumRamPartitions>
sProcessData;

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

void
initialiseRamPartitionConfig()
{
   // TODO: Check system flags to see if we should use DevRamPartitionConfig
   sRamPartitionConfig = RetailRamPartitionConfig;
}

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

ProcessData *
getCurrentProcessData()
{
   return &sProcessData[static_cast<size_t>(getCurrentRampid())];
}

RamPartitionId
getCurrentRampid()
{
   return RamPartitionId::MainApplication;
}

UniqueProcessId
getCurrentUpid()
{
   return UniqueProcessId::Game;
}

RamPartitionId
getRampidFromUpid(UniqueProcessId id)
{
   switch (id) {
   case UniqueProcessId::Kernel:
      return RamPartitionId::Kernel;
   case UniqueProcessId::Root:
      return RamPartitionId::Root;
   case UniqueProcessId::HomeMenu:
      return RamPartitionId::MainApplication;
   case UniqueProcessId::TV:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::EManual:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::OverlayMenu:
      return RamPartitionId::OverlayMenu;
   case UniqueProcessId::ErrorDisplay:
      return RamPartitionId::ErrorDisplay;
   case UniqueProcessId::MiniMiiverse:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::InternetBrowser:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::Miiverse:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::EShop:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::FLV:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::DownloadManager:
      return RamPartitionId::OverlayApp;
   case UniqueProcessId::Game:
      return RamPartitionId::MainApplication;
   default:
      decaf_abort(fmt::format("Unknown UniqueProcessId {}", static_cast<uint32_t>(id)));
   }
}

ProcessPerCoreStartInfo *
getProcessPerCoreStartInfo(uint32_t id)
{
   return &getCurrentProcessData()->perCoreStartInfo[id];
}

static void
initialisePerCoreStartInfo(ProcessPerCoreStartInfo *core0,
                           ProcessPerCoreStartInfo *core1,
                           ProcessPerCoreStartInfo *core2)
{
   auto processData = getCurrentProcessData();
   auto stackSize0 = std::max<uint32_t>(0x4000u, processData->startInfo.stackSize);
   auto stackSize1 = std::max<uint32_t>(0x4000u, processData->startInfo.stackSize);
   auto stackSize2 = std::max<uint32_t>(0x4000u, processData->startInfo.stackSize);
   auto exceptionStackSize0 = 0x1000u;
   auto exceptionStackSize1 = 0x1000u;
   auto exceptionStackSize2 = 0x1000u;

   core0->entryPoint = processData->startInfo.entryPoint;
   core1->entryPoint = processData->startInfo.entryPoint;
   core2->entryPoint = processData->startInfo.entryPoint;
   core0->sdaBase = processData->startInfo.sdaBase;
   core1->sdaBase = processData->startInfo.sdaBase;
   core2->sdaBase = processData->startInfo.sdaBase;
   core0->sda2Base = processData->startInfo.sda2Base;
   core1->sda2Base = processData->startInfo.sda2Base;
   core2->sda2Base = processData->startInfo.sda2Base;
   core0->unk0x1C = processData->startInfo.unk0x10;
   core1->unk0x1C = processData->startInfo.unk0x10;
   core2->unk0x1C = processData->startInfo.unk0x10;

   core0->stackEnd = align_up(processData->startInfo.dataAreaStart, 8);
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
   decaf_check(stacksEnd > processData->startInfo.dataAreaStart &&
               stacksEnd <= processData->startInfo.dataAreaEnd);

   processData->startInfo.dataAreaStart = stacksEnd;
}

loader::RPL_STARTINFO *
getProcessStartInfo()
{
   return &getCurrentProcessData()->startInfo;
}

void
loadGameProcess(std::string_view rpx,
                ios::mcp::MCPPPrepareTitleInfo &titleInfo)
{
   auto processData = getCurrentProcessData();
   auto rampid = RamPartitionId::MainApplication;
   processData->argstr = reinterpret_cast<char *>(std::addressof(titleInfo.argstr.at(0)));

   // Initialise memory
   allocateRamPartition(rampid,
                        titleInfo.max_size,
                        titleInfo.avail_size,
                        titleInfo.codegen_size,
                        titleInfo.max_codesize,
                        titleInfo.codegen_core,
                        &processData->ramPartitionAllocation);

   processData->overlayArenaEnabled = false;
   if (rampid == RamPartitionId::MainApplication) {
      processData->overlayArenaEnabled = !!titleInfo.overlay_arena;
   }

   internal::initialiseAddressSpace(&processData->addressSpace,
                                    rampid,
                                    processData->ramPartitionAllocation.codeStart,
                                    static_cast<uint32_t>(processData->ramPartitionAllocation.codeEnd - processData->ramPartitionAllocation.codeStart),
                                    processData->ramPartitionAllocation.dataStart,
                                    static_cast<uint32_t>(processData->ramPartitionAllocation.availStart - processData->ramPartitionAllocation.dataStart),
                                    0, 0,
                                    processData->ramPartitionAllocation.availStart,
                                    static_cast<uint32_t>(processData->ramPartitionAllocation.codeGenStart - processData->ramPartitionAllocation.availStart),
                                    processData->ramPartitionAllocation.codeGenStart,
                                    static_cast<uint32_t>(processData->ramPartitionAllocation.codeStart - processData->ramPartitionAllocation.codeGenStart),
                                    processData->ramPartitionAllocation.codegen_core,
                                    processData->overlayArenaEnabled);

   internal::loadAddressSpace(&processData->addressSpace);

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
   internal::KiRPLStartup(
      cafe::kernel::UniqueProcessId::Kernel,
      cafe::kernel::UniqueProcessId::Game,
      cafe::kernel::ProcessFlags::get(0).debugLevel(cafe::kernel::DebugLevel::Verbose),
      num_codearea_heap_blocks + num_workarea_heap_blocks,
      static_cast<uint32_t>(processData->ramPartitionAllocation.codeEnd - processData->ramPartitionAllocation.codeStart),
      static_cast<uint32_t>(processData->ramPartitionAllocation.availStart - processData->ramPartitionAllocation.dataStart),
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
         if (!sectionAddress  ||
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
   auto processData = getCurrentProcessData();
   auto loaderIpcStorage = cafe::loader::getKernelIpcStorage();
   processData->startInfo = loaderIpcStorage->startInfo;
   processData->loadedRpx = loaderIpcStorage->rpxModule;
   processData->loadedModuleList = loaderIpcStorage->loadedModuleList;

   initialisePerCoreStartInfo(&processData->perCoreStartInfo[0],
                              &processData->perCoreStartInfo[1],
                              &processData->perCoreStartInfo[2]);

   // KiProcess_Launch
   using EntryPointFn = virt_func_ptr<void()>;
   auto entryPoint = processData->perCoreStartInfo[cpu::this_core::id()].entryPoint;
   cafe::invoke(cpu::this_core::state(),
                virt_func_cast<EntryPointFn>(entryPoint));

}

void
exitProcess(int code)
{
   auto processData = getCurrentProcessData();
   processData->exitCode = code;
   internal::exit();
}

int
getProcessExitCode(RamPartitionId rampid)
{
   return sProcessData[static_cast<size_t>(rampid)].exitCode;
}

} // namespace cafe::kernel
