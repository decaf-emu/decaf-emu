#include "decaf_config.h"
#include "gx2.h"
#include "gx2_contextstate.h"
#include "gx2_displaylist.h"
#include "gx2_event.h"
#include "gx2_internal_cbpool.h"
#include "gx2_internal_pm4cap.h"
#include "gx2_state.h"
#include "cafe/libraries/coreinit/coreinit_core.h"
#include "cafe/libraries/coreinit/coreinit_memdefaultheap.h"

#include <common/log.h>
#include <common/platform_dir.h>
#include <libgpu/gpu.h>
#include <libgpu/latte/latte_pm4_commands.h>

namespace cafe::gx2
{

using namespace coreinit;

struct StaticStateData
{
   be2_val<uint32_t> mainCoreId;
   be2_array<BOOL, 3> profilingEnabled;
   be2_val<GX2ProfileMode> profileMode;
   be2_val<GX2TossStage> tossStage;
   be2_val<uint32_t> timeoutMS;
};

static virt_ptr<StaticStateData>
sStateData = nullptr;

void
GX2Init(virt_ptr<GX2InitAttrib> attributes)
{
   auto cbPoolBase = virt_ptr<uint32_t> { nullptr };
   auto argv = virt_ptr<char> { nullptr };
   auto cbPoolSize = 0x400000u;
   auto argc = 0u;
   auto profileMode = GX2ProfileMode::None;
   auto tossStage = GX2TossStage::None;

   // Set main gx2 core
   sStateData->mainCoreId = OSGetCoreId();

   // Set default GPU timeout to 10 seconds
   sStateData->timeoutMS = 10u * 1000;

   // Parse attributes
   while (attributes && *attributes != GX2InitAttrib::End) {
      auto id = *(attributes++);
      auto value = static_cast<uint32_t>(*(attributes++));

      switch (id) {
      case GX2InitAttrib::CommandBufferPoolBase:
         cbPoolBase = virt_cast<uint32_t *>(virt_addr { value });
         break;
      case GX2InitAttrib::CommandBufferPoolSize:
         cbPoolSize = value;
         break;
      case GX2InitAttrib::ArgC:
         argc = value;
         break;
      case GX2InitAttrib::ArgV:
         argv = virt_cast<char *>(virt_addr { value });
         break;
      case GX2InitAttrib::ProfileMode:
         profileMode = static_cast<GX2ProfileMode>(value);
         break;
      case GX2InitAttrib::TossStage:
         tossStage = static_cast<GX2TossStage>(value);
         break;
      default:
         gLog->warn("Unknown GX2InitAttrib {} = {}", id, value);
      }
   }

   // Ensure minimum size
   if (cbPoolSize < 0x2000) {
      cbPoolSize = 0x2000;
   }

   // Allocate command buffer pool
   if (!cbPoolBase) {
      cbPoolBase = virt_cast<uint32_t *>(MEMAllocFromDefaultHeapEx(cbPoolSize, 0x100));
   }

   // Init event handler stuff (vsync, flips, etc)
   internal::initEvents();

   // Initialise GPU callbacks
   gpu::setFlipCallback(&internal::onFlip);
   gpu::setSyncRegisterCallback(&internal::captureSyncGpuRegisters);
   gpu::setRetireCallback(&internal::onRetireCommandBuffer);

   // Initialise command buffer pools
   internal::initCommandBufferPool(cbPoolBase, cbPoolSize / 4);

   // Initialise profiling settings
   internal::initialiseProfiling(profileMode, tossStage);

   // Setup default gx2 state
   internal::disableStateShadowing();
   internal::initialiseRegisters();
   GX2SetDefaultState();
   GX2Flush();
}

void
GX2Shutdown()
{
}

void
GX2Flush()
{
   if (GX2GetDisplayListWriteStatus()) {
      gLog->error("GX2Flush called from within a display list");
   }

   internal::flushCommandBuffer(0x100);
}

uint32_t
GX2GetGPUTimeout()
{
   return sStateData->timeoutMS;
}

void
GX2SetGPUTimeout(uint32_t timeout)
{
   sStateData->timeoutMS = timeout;
}

namespace internal
{

void
enableStateShadowing()
{
   auto LOAD_CONTROL = latte::CONTEXT_CONTROL_ENABLE::get(0)
      .ENABLE_CONFIG_REG(true)
      .ENABLE_CONTEXT_REG(true)
      .ENABLE_ALU_CONST(true)
      .ENABLE_BOOL_CONST(true)
      .ENABLE_LOOP_CONST(true)
      .ENABLE_RESOURCE(true)
      .ENABLE_SAMPLER(true)
      .ENABLE_CTL_CONST(true)
      .ENABLE_ORDINAL(true);

   auto SHADOW_ENABLE = latte::CONTEXT_CONTROL_ENABLE::get(0)
      .ENABLE_CONFIG_REG(true)
      .ENABLE_CONTEXT_REG(true)
      .ENABLE_ALU_CONST(true)
      .ENABLE_BOOL_CONST(true)
      .ENABLE_LOOP_CONST(true)
      .ENABLE_RESOURCE(true)
      .ENABLE_SAMPLER(true)
      .ENABLE_CTL_CONST(true)
      .ENABLE_ORDINAL(true);

   internal::writePM4(latte::pm4::ContextControl {
      LOAD_CONTROL,
      SHADOW_ENABLE
   });
}

void
disableStateShadowing()
{
   auto LOAD_CONTROL = latte::CONTEXT_CONTROL_ENABLE::get(0)
      .ENABLE_ORDINAL(true);

   auto SHADOW_ENABLE = latte::CONTEXT_CONTROL_ENABLE::get(0)
      .ENABLE_ORDINAL(true);

   internal::writePM4(latte::pm4::ContextControl {
      LOAD_CONTROL,
      SHADOW_ENABLE
   });
}

bool
isInitialised()
{
   return sStateData->mainCoreId != 0xFF;
}

uint32_t
getMainCoreId()
{
   return sStateData->mainCoreId;
}

void
setMainCore()
{
   sStateData->mainCoreId = OSGetCoreId();
}

void
initialiseProfiling(GX2ProfileMode profileMode,
                    GX2TossStage tossStage)
{
   sStateData->profileMode = profileMode;
   sStateData->tossStage = tossStage;

   // TODO: Update these GX2ProfileMode values with enum named values
   switch (tossStage) {
   case 1:
      sStateData->profileMode |= 0x60;
      break;
   case 2:
      sStateData->profileMode |= 0x40;
      break;
   case 7:
      sStateData->profileMode |= 0x90;
      break;
   case 8:
      sStateData->profileMode |= 0x10;
      break;
   }

   sStateData->profilingEnabled[0] = true;
   sStateData->profilingEnabled[1] = true;
   sStateData->profilingEnabled[2] = true;
}

GX2ProfileMode
getProfileMode()
{
   return sStateData->profileMode;
}

GX2TossStage
getTossStage()
{
   return sStateData->tossStage;
}

BOOL
getProfilingEnabled()
{
   return sStateData->profilingEnabled[cpu::this_core::id()];
}

void
setProfilingEnabled(BOOL enabled)
{
   sStateData->profilingEnabled[cpu::this_core::id()] = enabled;
}

} // namespace internal

void
Library::registerStateSymbols()
{
   RegisterFunctionExport(GX2Init);
   RegisterFunctionExport(GX2Shutdown);
   RegisterFunctionExport(GX2Flush);
   RegisterFunctionExport(GX2GetGPUTimeout);
   RegisterFunctionExport(GX2SetGPUTimeout);

   RegisterDataInternal(sStateData);
}

} // namespace cafe::gx2
