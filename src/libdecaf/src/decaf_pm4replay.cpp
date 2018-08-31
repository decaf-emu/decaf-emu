#include "decaf_pm4replay.h"
#include "cafe/libraries/gx2/gx2_internal_cbpool.h"

namespace decaf
{

namespace pm4
{

void
injectCommandBuffer(void *buffer,
                    uint32_t bytes)
{
   cafe::gx2::internal::queueDisplayList(reinterpret_cast<uint32_t *>(buffer), bytes / 4);
}

} // namespace pm4

} // namespace decaf
