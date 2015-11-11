#include "pm4.h"
#include "modules/gx2/gx2_cbpool.h"

namespace pm4
{

CommandBuffer *
getCommandBuffer(uint32_t size)
{
   return gx2::internal::getCommandBuffer(size);
}

CommandBuffer *
flushCommandBuffer(CommandBuffer *buffer)
{
   return gx2::internal::flushCommandBuffer(buffer);
}

} // namespace pm4
