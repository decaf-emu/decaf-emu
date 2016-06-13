#include <algorithm>
#include "coreinit.h"
#include "coreinit_core.h"
#include "coreinit_rendezvous.h"

namespace coreinit
{


/**
 * Initialise a rendezvous structure.
 */
void
OSInitRendezvous(OSRendezvous *rendezvous)
{
   std::fill(std::begin(rendezvous->core), std::end(rendezvous->core), 0);
   std::fill(std::begin(rendezvous->__unk0), std::end(rendezvous->__unk0), 0);
}


/**
 * Wait on a rendezvous with infinite timeout.
 */
BOOL
OSWaitRendezvous(OSRendezvous *rendezvous,
                 uint32_t coreMask)
{
   return OSWaitRendezvousWithTimeout(rendezvous, coreMask, -1);
}


/**
 * Wait on a rendezvous with a timeout.
 *
 * This will wait with a timeout until all cores matching coreMask have
 * reached the rendezvous point.
 *
 * \return Returns TRUE on success, FALSE on timeout.
 */
BOOL
OSWaitRendezvousWithTimeout(OSRendezvous *rendezvous,
                            uint32_t coreMask,
                            OSTime timeout)
{
   auto core = OSGetCoreId();
   auto success = FALSE;
   auto endTime = OSGetTime() + timeout;

   // Set our core flag
   rendezvous->core[core].store(1, std::memory_order_release);

   do {
      success = TRUE;

      // Check all core flags
      for (auto i = 0u; i < 3; ++i) {
         if (coreMask & (1 << i)) {
            if (!rendezvous->core[i].load(std::memory_order_acquire)) {
               success = FALSE;
            }
         }
      }

      // Check for timeout
      if (timeout != -1 && OSGetTime() >= endTime) {
         break;
      }
   } while (!success);

   return success;
}


void
Module::registerRendezvousFunctions()
{
   RegisterKernelFunction(OSInitRendezvous);
   RegisterKernelFunction(OSWaitRendezvous);
   RegisterKernelFunction(OSWaitRendezvousWithTimeout);
}

} // namespace coreinit
