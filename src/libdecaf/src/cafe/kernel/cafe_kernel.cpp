#include "cafe_kernel.h"
#include "cafe_kernel_context.h"
#include "cafe_kernel_exception.h"
#include "cafe_kernel_heap.h"
#include "cafe_kernel_ipckdriver.h"
#include "cafe_kernel_lock.h"
#include "cafe_kernel_loader.h"
#include "cafe_kernel_mmu.h"
#include "cafe_kernel_process.h"
#include "cafe_kernel_shareddata.h"

#include "cafe/libraries/cafe_hle.h"
#include "decaf_events.h"
#include "decaf_game.h"
#include "kernel/kernel_filesystem.h"
#include "kernel/kernel_gameinfo.h"
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
};

static virt_ptr<StaticKernelData>
sKernelData = nullptr;

static internal::AddressSpace
sKernelAddressSpace;

static std::array<virt_ptr<Context>, 3>
sSubCoreEntryContexts = { };

static std::atomic<bool>
sStopping { false };

static decaf::GameInfo
sGameInfo;

static std::string
sExecutableName;

static void
mainCoreEntryPoint(cpu::Core *core)
{
   internal::initialiseCoreContext(core);
   internal::initialiseExceptionContext(core);
   internal::initialiseExceptionHandlers();
   internal::ipckDriverInit();
   internal::ipckDriverOpen();

   // TODO: This is normally called by root.rpx
   loadShared();

   // TODO: Game information should come from ios MCPPPrepareTitleInfo!
   if (!::kernel::loadGameInfo(sGameInfo)) {
      gLog->warn("Could not load game info.");
   } else {
      gLog->info("Loaded game info: '{}' - {} v{}",
                 sGameInfo.meta.shortnames[decaf::Language::English],
                 sGameInfo.meta.product_code,
                 sGameInfo.meta.title_version);
   }

   if (sGameInfo.cos.argstr.empty()) {
      sGameInfo.cos.argstr = sExecutableName;
   }

   // TODO: This should be inside IOS
   // Mount sdcard if the game has the appropriate permissions
   if ((sGameInfo.cos.permission_fs & decaf::CosXML::FSPermission::SdCardRead) ||
       (sGameInfo.cos.permission_fs & decaf::CosXML::FSPermission::SdCardWrite)) {
      auto filesystem = ::kernel::getFileSystem();

      // Ensure sdcard_path exists
      platform::createDirectory(decaf::config::system::sdcard_path);

      auto sdcardPath = fs::HostPath { decaf::config::system::sdcard_path };
      auto permissions = fs::Permissions::Read;

      if (sGameInfo.cos.permission_fs & decaf::CosXML::FSPermission::SdCardWrite) {
         permissions = fs::Permissions::ReadWrite;
      }

      filesystem->mountHostFolder("/dev/sdcard01", sdcardPath, permissions);
   }

   const auto &rpx = sGameInfo.cos.argstr;
   if (rpx.empty()) {
      gLog->error("Could not find game executable to load.");
      return;
   }

   decaf::event::onGameLoaded(sGameInfo);

   // Load and run the game
   ios::mcp::MCPPPrepareTitleInfo titleInfo;
   titleInfo.version = static_cast<uint32_t>(sGameInfo.cos.version);
   titleInfo.titleId = static_cast<uint64_t>(sGameInfo.app.title_id);
   titleInfo.cmdFlags = static_cast<uint32_t>(sGameInfo.cos.cmdFlags);
   titleInfo.argstr = sGameInfo.cos.argstr;
   titleInfo.max_size = static_cast<uint32_t>(sGameInfo.cos.max_size);
   titleInfo.avail_size = static_cast<uint32_t>(sGameInfo.cos.avail_size);
   titleInfo.codegen_size = static_cast<uint32_t>(sGameInfo.cos.codegen_size);
   titleInfo.codegen_core = static_cast<uint32_t>(sGameInfo.cos.codegen_core);
   titleInfo.max_codesize = static_cast<uint32_t>(sGameInfo.cos.max_codesize);
   titleInfo.overlay_arena = static_cast<uint32_t>(sGameInfo.cos.overlay_arena);
   titleInfo.num_workarea_heap_blocks = static_cast<uint32_t>(sGameInfo.cos.num_workarea_heap_blocks);
   titleInfo.num_codearea_heap_blocks = static_cast<uint32_t>(sGameInfo.cos.num_codearea_heap_blocks);
   titleInfo.default_stack0_size = static_cast<uint32_t>(sGameInfo.cos.default_stack0_size);
   titleInfo.default_stack1_size = static_cast<uint32_t>(sGameInfo.cos.default_stack1_size);
   titleInfo.default_stack2_size = static_cast<uint32_t>(sGameInfo.cos.default_stack2_size);
   titleInfo.default_redzone0_size = static_cast<uint32_t>(sGameInfo.cos.default_redzone0_size);
   titleInfo.default_redzone1_size = static_cast<uint32_t>(sGameInfo.cos.default_redzone1_size);
   titleInfo.default_redzone2_size = static_cast<uint32_t>(sGameInfo.cos.default_redzone2_size);
   titleInfo.exception_stack0_size = static_cast<uint32_t>(sGameInfo.cos.exception_stack0_size);
   titleInfo.exception_stack1_size = static_cast<uint32_t>(sGameInfo.cos.exception_stack1_size);
   titleInfo.exception_stack2_size = static_cast<uint32_t>(sGameInfo.cos.exception_stack2_size);

   for (auto i = 0u; i < titleInfo.permissions.size(); ++i) {
      titleInfo.permissions[i].group = static_cast<uint32_t>(sGameInfo.cos.permissions[i].group);
      titleInfo.permissions[i].mask = static_cast<uint64_t>(sGameInfo.cos.permissions[i].mask);
   }

   titleInfo.sdkVersion = static_cast<uint32_t>(sGameInfo.app.sdk_version);
   titleInfo.titleVersion = static_cast<uint32_t>(sGameInfo.app.title_version);

   loadGameProcess(rpx, titleInfo);
   finishInitAndPreload();
}

static void
subCoreEntryPoint(cpu::Core *core)
{
   internal::initialiseCoreContext(core);
   internal::initialiseExceptionContext(core);
   internal::ipckDriverInit();
   internal::ipckDriverOpen();

   while (!sStopping.load()) {
      internal::kernelLockAcquire();
      auto entryContext = sSubCoreEntryContexts[core->id];
      internal::kernelLockRelease();

      if (entryContext) {
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
