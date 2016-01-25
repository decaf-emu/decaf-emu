#include "gpu/driver.h"
#include "gpu/pm4.h"
#include "gx2_cbpool.h"
#include "gx2_contextstate.h"
#include "gx2_displaylist.h"
#include "gx2_event.h"
#include "gx2_state.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "utils/log.h"
#include "utils/virtual_ptr.h"
#include "utils/wfunc_call.h"

namespace gx2
{

static uint32_t
gMainCoreId = 0;

void
GX2Init(be_val<uint32_t> *attributes)
{
   virtual_ptr<uint32_t> cbPoolBase = nullptr;
   uint32_t cbPoolSize = 0x400000;
   uint32_t cbPoolItemSize = 0x100;
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
   gx2::internal::initEvents();

   // Initialise command buffer pools
   gx2::internal::initCommandBufferPool(cbPoolBase, cbPoolSize, cbPoolItemSize);

   // Start our driver!
   gpu::driver::start();

   // Setup default gx2 state
   GX2SetDefaultState();
}

void
GX2Shutdown()
{
   gpu::driver::destroy();
}

void
GX2Flush()
{
   if (GX2GetDisplayListWriteStatus()) {
      gLog->error("GX2Flush called from within a display list");
   }

   gx2::internal::flushCommandBuffer(nullptr);
}

namespace internal
{

uint32_t getMainCoreId()
{
   return gMainCoreId;
}

} // namespace internal

} // namespace gx2
