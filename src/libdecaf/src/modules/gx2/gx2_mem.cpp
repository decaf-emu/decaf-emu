#include "gx2_mem.h"
#include "gx2_state.h"
#include "gpu/commandqueue.h"
#include "gpu/pm4.h"

namespace gx2
{

void
GX2Invalidate(GX2InvalidateMode mode,
              void *buffer,
              uint32_t size)
{
   if (!internal::isInitialised()) {
      return;
   }

   auto memStart = mem::untranslate(buffer);
   auto memEnd = memStart + size;

   // Custom build the packet so we can send it through
   //  the system packet path rather than the normal one.
   auto packet = new be_val<uint32_t>[4];
   packet[0] = pm4::type3::Header::get(0)
      .type(pm4::Header::Type3)
      .opcode(pm4::DecafInvalidate::Opcode)
      .size(4 - 2)
      .value;
   packet[1] = mode;
   packet[2] = memStart;
   packet[3] = memEnd;

   gpu::queueSysBuffer(packet, 4);
}

} // namespace gx2
