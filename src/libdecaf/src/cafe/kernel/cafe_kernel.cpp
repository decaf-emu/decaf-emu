#include "cafe_kernel.h"
#include "cafe_kernel_context.h"
#include "cafe_kernel_exception.h"
#include "cafe_kernel_heap.h"
#include "cafe_kernel_ipckdriver.h"
#include "cafe_kernel_ipc.h"
#include "cafe_kernel_lock.h"
#include "cafe_kernel_loader.h"
#include "cafe_kernel_mcp.h"
#include "cafe_kernel_mmu.h"
#include "cafe_kernel_process.h"
#include "cafe_kernel_shareddata.h"

#include "cafe/libraries/cafe_hle.h"
#include "decaf_events.h"
#include "decaf_game.h"
#include "kernel/kernel_filesystem.h"
#include "ios/mcp/ios_mcp_mcp_types.h"

#include <atomic>
#include <common/log.h>
#include <common/platform_dir.h>

namespace cafe::kernel
{

struct StaticKernelData
{
   struct CoreData
   {
      // Used for cpu branch trace handler
      be2_val<uint32_t> symbolDistance;
      be2_array<char, 256> symbolNameBuffer;
      be2_array<char, 256> moduleNameBuffer;
   };

   be2_array<CoreData, 3> coreData;
   be2_struct<ios::mcp::MCPPPrepareTitleInfo> prepareTitleInfo;
};

static virt_ptr<StaticKernelData>
sKernelData = nullptr;

static internal::AddressSpace
sKernelAddressSpace;

static std::array<virt_ptr<Context>, 3>
sSubCoreEntryContexts = { };

static std::atomic<bool>
sStopping { false };

static std::string
sExecutableName;

static void
mainCoreEntryPoint(cpu::Core *core)
{
   internal::setActiveAddressSpace(&sKernelAddressSpace);
   internal::initialiseCoreContext(core);
   internal::initialiseExceptionContext(core);
   internal::initialiseExceptionHandlers();

   // Set all cores to kernel process
   for (auto i = 0; i < 3; ++i) {
      internal::initialiseCoreProcess(i,
                                      RamPartitionId::Kernel,
                                      UniqueProcessId::Kernel,
                                      KernelProcessId::Invalid);
   }

   internal::initialiseProcessData();
   internal::ipckDriverInit();
   internal::ipckDriverOpen();
   internal::initialiseIpc();

   // TODO: This is normally called by root.rpx
   // TODO: Change this to use IPC file read commands.
   loadShared();

   // Prepare title
   auto titleInfo = virt_addrof(sKernelData->prepareTitleInfo);
   if (auto error = internal::mcpPrepareTitle(ios::mcp::DefaultTitleId,
                                              titleInfo)) {
      // Not a full title - fill out some default values!
      titleInfo->version = 1u;
      titleInfo->cmdFlags = 0u;
      titleInfo->avail_size = 0u;
      titleInfo->codegen_size = 0u;
      titleInfo->codegen_core = 1u;
      titleInfo->max_size = 0x40000000u;
      titleInfo->max_codesize = 0x0E000000u;
      titleInfo->default_stack0_size = 0u;
      titleInfo->default_stack1_size = 0u;
      titleInfo->default_stack2_size = 0u;
      titleInfo->exception_stack0_size = 0x1000u;
      titleInfo->exception_stack1_size = 0x1000u;
      titleInfo->exception_stack2_size = 0x1000u;

      std::strncpy(virt_addrof(titleInfo->argstr).getRawPointer(),
                   sExecutableName.data(),
                   sExecutableName.size());
   } else {
      gLog->info("Loaded title {:016X}, argstr \"{}\"",
                 titleInfo->titleId,
                 virt_addrof(titleInfo->argstr).getRawPointer());
   }

   auto rpx = std::string_view { virt_addrof(titleInfo->argstr).getRawPointer() };
   if (rpx.empty()) {
      gLog->error("Could not find game executable to load.");
      return;
   }

   // Perform the initial load
   internal::loadGameProcess(rpx, titleInfo);

   // Notify front end that game is loaded
   auto gameInfo = decaf::GameInfo { };
   gameInfo.titleId = titleInfo->titleId;
   if (auto pos = rpx.find_first_of(' '); pos != std::string_view::npos) {
      gameInfo.executable = rpx.substr(0, pos);
   } else {
      gameInfo.executable = rpx;
   }
   decaf::event::onGameLoaded(gameInfo);

   // Start the game
   internal::finishInitAndPreload();
}

static void
subCoreEntryPoint(cpu::Core *core)
{
   internal::setActiveAddressSpace(&sKernelAddressSpace);
   internal::initialiseCoreContext(core);
   internal::initialiseExceptionContext(core);
   internal::ipckDriverInit();
   internal::ipckDriverOpen();

   while (!sStopping.load()) {
      internal::kernelLockAcquire();
      auto entryContext = sSubCoreEntryContexts[core->id];
      internal::kernelLockRelease();

      if (entryContext) {
         // Set the core's current process to the main application
         internal::setCoreToProcessId(RamPartitionId::MainApplication,
                                      KernelProcessId::Kernel);
         internal::initialiseCoreProcess(core->id,
                                         RamPartitionId::MainApplication,
                                         UniqueProcessId::Game,
                                         KernelProcessId::Kernel);

         switchContext(entryContext);
         break;
      }

      cpu::this_core::waitNextInterrupt();
   }
}

void
setSubCoreEntryContext(int coreId,
                       virt_ptr<Context> context)
{
   internal::kernelLockAcquire();
   sSubCoreEntryContexts[coreId] = context;
   internal::kernelLockRelease();

   cpu::interrupt(coreId, cpu::GENERIC_INTERRUPT);
}

static void
cpuEntrypoint(cpu::Core *core)
{
   if (core->id == 1) {
      mainCoreEntryPoint(core);
   } else {
      subCoreEntryPoint(core);
   }

   internal::idleCoreLoop(core);
}

static void
cpuBranchTraceHandler(cpu::Core *core,
                      uint32_t target)
{
   if (decaf::config::log::branch_trace) {
      auto &data = sKernelData->coreData[core->id];
      auto error =
         internal::findClosestSymbol(virt_addr { target },
                                     virt_addrof(data.symbolDistance),
                                     virt_addrof(data.symbolNameBuffer),
                                     data.symbolNameBuffer.size(),
                                     virt_addrof(data.moduleNameBuffer),
                                     data.moduleNameBuffer.size());

      if (!error && data.moduleNameBuffer[0] && data.symbolNameBuffer[0]) {
         gLog->trace("CPU branched to: 0x{:08X} {}|{}+0x{:X}",
                     target,
                     virt_addrof(data.moduleNameBuffer).getRawPointer(),
                     virt_addrof(data.symbolNameBuffer).getRawPointer(),
                     data.symbolDistance);
      } else {
         gLog->trace("CPU branched to: 0x{:08X}", target);
      }
   }
}

static void
cpuKernelCallHandler(cpu::Core *core,
                     uint32_t id)
{
   // Save our original stack pointer for the backchain
   auto backchainSp = core->gpr[1];

   // Allocate callee backchain and lr space.
   core->gpr[1] -= 2 * 4;

   // Write the backchain pointer
   *virt_cast<uint32_t *>(virt_addr { core->gpr[1] }) = backchainSp;

   // Handle the HLE function call
   cafe::hle::Library::handleKernelCall(core, id);

   // Grab the most recent core state as it may have changed.
   core = cpu::this_core::state();

   // Release callee backchain and lr space.
   core->gpr[1] += 2 * 4;
}

void
start()
{
   hle::initialiseLibraries();

   // Initialise memory
   internal::initialiseAddressSpace(&sKernelAddressSpace,
                                    RamPartitionId::Kernel,
                                    phys_addr { 0x72000000 }, 0x0E000000,
                                    phys_addr { 0x20000000 }, 0x52000000,
                                    0, 0,
                                    phys_addr { 0 }, 0,
                                    phys_addr { 0 }, 0,
                                    0, false);
   internal::loadAddressSpace(&sKernelAddressSpace);
   internal::initialiseStaticDataHeap();

   // Initialise static data
   sKernelData = internal::allocStaticData<StaticKernelData>();
   internal::initialiseStaticContextData();
   internal::initialiseStaticExceptionData();
   internal::initialiseStaticIpckDriverData();
   internal::initialiseStaticIpcData();

   // Setup cpu
   cpu::setCoreEntrypointHandler(&cpuEntrypoint);

   if (decaf::config::log::branch_trace) {
      cpu::setBranchTraceHandler(&cpuBranchTraceHandler);
   }

   cpu::setKernelCallHandler(&cpuKernelCallHandler);

   // Start the cpu
   cpu::start();
}

bool
hasExited()
{
   return sStopping;
}

void
join()
{
   if (!sStopping) {
      sStopping = true;
      cpu::halt();
   }

   cpu::join();
}

void
setExecutableFilename(const std::string& name)
{
   sExecutableName = name;
}

namespace internal
{

void
idleCoreLoop(cpu::Core *core)
{
   // Set up the default expected state for the nia/cia of idle threads.
   //  This must be kept in sync with reschedule which sets them to this
   //  for debugging purposes.
   core->nia = 0xFFFFFFFF;
   core->cia = 0xFFFFFFFF;

   while (!sStopping.load()) {
      cpu::this_core::waitForInterrupt();
   }

   gLog->info("Core {} exit", core->id);
}

void
exit()
{
   // Set the running flag to false so idle loops exit.
   sStopping = true;

   // Tell the CPU to stop.
   cpu::halt();

   // Switch to idle context to prevent further execution.
   switchContext(nullptr);
}

} // namespace internal

} // namespace cafe::kernel
