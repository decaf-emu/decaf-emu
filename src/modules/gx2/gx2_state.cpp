#include "gpu/pm4.h"
#include "gx2_cbpool.h"
#include "gx2_state.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "utils/virtual_ptr.h"

void
GX2Init(be_val<uint32_t> *attributes)
{
   virtual_ptr<uint32_t> cbPoolBase = nullptr;
   uint32_t cbPoolSize = 0x400000;
   uint32_t cbPoolItemSize = 0x100;
   virtual_ptr<char *> argv = nullptr;
   uint32_t argc = 0;

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
      cbPoolBase = reinterpret_cast<uint32_t*>((*pMEMAllocFromDefaultHeapEx)(cbPoolSize, 0x100));
   }

   // Initialise command buffer pools
   gx2::internal::initCommandBufferPool(cbPoolBase, cbPoolSize, cbPoolItemSize);
}

void
GX2Shutdown()
{
}

void
GX2Flush()
{
}
