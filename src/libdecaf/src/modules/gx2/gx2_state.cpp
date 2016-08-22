#include "gpu/pm4.h"
#include "gx2_cbpool.h"
#include "gx2_contextstate.h"
#include "gx2_displaylist.h"
#include "gx2_event.h"
#include "gx2_state.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "common/log.h"
#include "virtual_ptr.h"
#include "ppcutils/wfunc_call.h"
#include "gpu/pm4_writer.h"

namespace gx2
{

static uint32_t
gMainCoreId = 0xFF;

void
GX2Init(be_val<uint32_t> *attributes)
{
   virtual_ptr<uint32_t> cbPoolBase = nullptr;
   uint32_t cbPoolSize = 0x400000;
   virtual_ptr<char *> argv = nullptr;
   uint32_t argc = 0;

   // Set main gx2 core
   gMainCoreId = coreinit::OSGetCoreId();

   // Parse attributes
   while (attributes && *attributes != GX2InitAttrib::End) {
      uint32_t id = *(attributes++);
      uint32_t value = *(attributes++);

      switch (id) {
      case GX2InitAttrib::CommandBufferPoolBase:
         cbPoolBase = make_virtual_ptr<uint32_t>(value);
         break;
      case GX2InitAttrib::CommandBufferPoolSize:
         cbPoolSize = value;
         break;
      case GX2InitAttrib::ArgC:
         argc = value;
         break;
      case GX2InitAttrib::ArgV:
         argv = make_virtual_ptr<char *>(value);
         break;
      }
   }

   // Ensure minimum size
   if (cbPoolSize < 0x2000) {
      cbPoolSize = 0x2000;
   }

   // Allocate command buffer pool
   if (!cbPoolBase) {
      cbPoolBase = reinterpret_cast<uint32_t*>((*coreinit::pMEMAllocFromDefaultHeapEx)(cbPoolSize, 0x100));
   }

   // Init event handler stuff (vsync, flips, etc)
   internal::initEvents();

   // Initialise command buffer pools
   internal::initCommandBufferPool(cbPoolBase, cbPoolSize / 4);

   // Setup default gx2 state
   internal::disableStateShadowing();
   internal::initRegisters();
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

   gx2::internal::flushCommandBuffer(0x100);
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

   pm4::write(pm4::ContextControl {
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

   pm4::write(pm4::ContextControl {
      LOAD_CONTROL,
      SHADOW_ENABLE
   });
}

bool
isInitialised()
{
   return gMainCoreId != 0xFF;
}

uint32_t
getMainCoreId()
{
   return gMainCoreId;
}

} // namespace internal

} // namespace gx2
