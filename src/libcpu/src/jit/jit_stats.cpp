#include "jit.h"
#include "jit_stats.h"

namespace cpu
{

namespace jit
{

bool
sampleStats(JitStats &stats)
{
   auto backend = getBackend();

   if (backend) {
      return backend->sampleStats(stats);
   } else {
      return false;
   }
}

void
resetProfileStats()
{
   auto backend = getBackend();

   if (backend) {
      backend->resetProfileStats();
   }
}

void
setProfilingMask(unsigned mask)
{
   auto backend = getBackend();

   if (backend) {
      backend->setProfilingMask(mask);
   }
}

unsigned
getProfilingMask()
{
   auto backend = getBackend();

   if (backend) {
      return backend->getProfilingMask();
   } else {
      return 0;
   }
}

} // namespace jit

} // namespace cpu
