#include "null_driver.h"
#include "gpu_event.h"
#include "gpu_ringbuffer.h"

#include <common/decaf_assert.h>

namespace null
{

void
Driver::setWindowSystemInfo(const gpu::WindowSystemInfo &wsi)
{
}

void
Driver::windowHandleChanged(void *handle)
{
}

void
Driver::windowSizeChanged(int width, int height)
{
}

void
Driver::run()
{
   mRunning = true;

   while (mRunning) {
      if (gpu::ringbuffer::wait()) {
         auto items = gpu::ringbuffer::read();
         // TODO: We need to actually process pm4 to do EVENT_WRITE_EOP for retired timestamps
      }
   }
}

void
Driver::runUntilFlip()
{
   decaf_abort("NullDriver::runUntilFlip unimplemented");
}

void
Driver::stop()
{
   mRunning = false;
   gpu::ringbuffer::wake();
}

gpu::GraphicsDriverType
Driver::type()
{
   return gpu::GraphicsDriverType::Null;
}

gpu::GraphicsDriverDebugInfo *
Driver::getDebugInfo()
{
   return nullptr;
}

void
Driver::notifyCpuFlush(phys_addr address,
                       uint32_t size)
{
}

void
Driver::notifyGpuFlush(phys_addr address,
                       uint32_t size)
{
}

} // namespace null
