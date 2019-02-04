#include "gx2.h"
#include "gx2_contextstate.h"
#include "gx2_debugcapture.h"
#include "gx2_displaylist.h"
#include "gx2_event.h"
#include "gx2_cbpool.h"
#include "gx2_internal_pm4cap.h"
#include "gx2_registers.h"
#include "gx2_state.h"

#include "cafe/libraries/coreinit/coreinit_core.h"
#include "cafe/libraries/coreinit/coreinit_memdefaultheap.h"
#include "cafe/libraries/tcl/tcl_driver.h"
#include "decaf_config.h"

#include <common/log.h>
#include <common/platform_dir.h>
#include <libgpu/gpu.h>
#include <libgpu/latte/latte_pm4_commands.h>

namespace cafe::gx2
{

using namespace coreinit;

struct StaticStateData
{
   be2_val<bool> initialized;
   be2_val<uint32_t> mainCoreId;
   be2_array<BOOL, 3> profilingEnabled;
   be2_val<GX2ProfileMode> profileMode;
   be2_val<GX2TossStage> tossStage;
   be2_val<uint32_t> timeoutMS;
   be2_val<uint32_t> hangState;
   be2_val<uint32_t> hangResponse;
   be2_val<uint32_t> hangResetSwapTimeout;
   be2_val<uint32_t> hangResetSwapsOutstanding;
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
   auto appIoThreadStackSize = 4096u;

   // Set main gx2 core
   sStateData->initialized = true;
   sStateData->mainCoreId = OSGetCoreId();

   // Set default GPU timeout to 10 seconds
   sStateData->timeoutMS = 10u * 1000;

   // Setup hang params
   GX2SetMiscParam(GX2MiscParam::HangResponse, 1);
   GX2SetMiscParam(GX2MiscParam::HangResetSwapTimeout, 1000);
   GX2SetMiscParam(GX2MiscParam::HangResetSwapsOutstanding, 3);

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
      case GX2InitAttrib::AppIoThreadStackSize:
         appIoThreadStackSize = value;
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

   // Allocate AppIo stack from end of cbPool
   auto appIoStackBuffer = align_up(virt_cast<virt_addr>(cbPoolBase) + cbPoolSize - appIoThreadStackSize, 64);
   appIoThreadStackSize = static_cast<uint32_t>(virt_cast<virt_addr>(cbPoolBase) + cbPoolSize - appIoStackBuffer);
   cbPoolSize = static_cast<uint32_t>(appIoStackBuffer - virt_cast<virt_addr>(cbPoolBase));

   // Init event handler stuff (vsync, flips, etc)
   internal::initEvents(virt_cast<void *>(appIoStackBuffer),
                        appIoThreadStackSize);

   // Initialise GPU callbacks
   gpu::setFlipCallback(&internal::onFlip);
   gpu::setSyncRegisterCallback(&internal::captureSyncGpuRegisters);

   // Initialise command buffer pools
   internal::initialiseCommandBufferPool(cbPoolBase, cbPoolSize);

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
   if (internal::debugCaptureEnabled()) {
      internal::debugCaptureShutdown();
   }
}

int32_t
GX2GetMainCoreId()
{
   if (sStateData->initialized) {
      return sStateData->mainCoreId;
   } else {
      return -1;
   }
}

void
GX2Flush()
{
   if (GX2GetDisplayListWriteStatus()) {
      gLog->error("GX2Flush called from within a display list");
   }

   internal::flushCommandBuffer(0x100, TRUE);
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

uint32_t
GX2GetMiscParam(GX2MiscParam param)
{
   switch (param) {
   case GX2MiscParam::HangState:
      return sStateData->hangState;
   case GX2MiscParam::HangResponse:
      return sStateData->hangResponse;
      break;
   case GX2MiscParam::HangResetSwapTimeout:
      return sStateData->hangResetSwapTimeout;
      break;
   case GX2MiscParam::HangResetSwapsOutstanding:
      return sStateData->hangResetSwapsOutstanding;
      break;
   default:
      return -1;
   }
}

BOOL
GX2SetMiscParam(GX2MiscParam param,
                uint32_t value)
{
   switch (param) {
   case GX2MiscParam::HangState:
      sStateData->hangState = value;
      break;
   case GX2MiscParam::HangResponse:
      if (value > 2) {
         return FALSE;
      }

      tcl::TCLSetHangWait(value == 1 ? TRUE : FALSE);
      sStateData->hangResponse = value;
      break;
   case GX2MiscParam::HangResetSwapTimeout:
      sStateData->hangResetSwapTimeout = value;
      break;
   case GX2MiscParam::HangResetSwapsOutstanding:
      sStateData->hangResetSwapsOutstanding = value;
      break;
   default:
      return FALSE;
   }

   return TRUE;
}

void
GX2SetSpecialState(GXSpecialState state,
                   BOOL enabled)
{
   if (state == GXSpecialState::Clear) {
      if (enabled) {
         internal::writePM4(latte::pm4::SetContextReg {
            latte::Register::PA_CL_VTE_CNTL,
            latte::PA_CL_VTE_CNTL::get(0)
               .VTX_XY_FMT(true)
               .VTX_Z_FMT(true)
               .value
         });
         internal::writePM4(latte::pm4::SetContextReg {
            latte::Register::PA_CL_CLIP_CNTL,
            latte::PA_CL_CLIP_CNTL::get(0)
               .CLIP_DISABLE(true)
               .DX_CLIP_SPACE_DEF(true)
               .value
         });
      } else {
         internal::writePM4(latte::pm4::SetContextReg {
            latte::Register::PA_CL_VTE_CNTL,
            latte::PA_CL_VTE_CNTL::get(0)
               .VPORT_X_SCALE_ENA(true)
               .VPORT_X_OFFSET_ENA(true)
               .VPORT_Y_SCALE_ENA(true)
               .VPORT_Y_OFFSET_ENA(true)
               .VPORT_Z_SCALE_ENA(true)
               .VPORT_Z_OFFSET_ENA(true)
               .VTX_W0_FMT(true)
               .value
         });
         GX2SetRasterizerClipControl(true, true);
      }
   } else if (state == GXSpecialState::Copy) {
      // There are no registers set for this special state.
   } else {
      gLog->warn("Unexpected GX2SetSpecialState state {}", state);
   }
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
   RegisterFunctionExport(GX2GetMainCoreId);
   RegisterFunctionExport(GX2Flush);
   RegisterFunctionExport(GX2GetGPUTimeout);
   RegisterFunctionExport(GX2SetGPUTimeout);
   RegisterFunctionExport(GX2GetMiscParam);
   RegisterFunctionExport(GX2SetMiscParam);
   RegisterFunctionExport(GX2SetSpecialState);

   RegisterDataInternal(sStateData);
}

} // namespace cafe::gx2
