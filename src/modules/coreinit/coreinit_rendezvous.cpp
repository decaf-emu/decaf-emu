#include "coreinit.h"
#include "coreinit_core.h"
#include "coreinit_rendezvous.h"

void
OSInitRendezvous(OSRendezvous *rendezvous)
{
   memset(rendezvous, 0, sizeof(OSRendezvous));
}

BOOL
OSWaitRendezvous(OSRendezvous *rendezvous, uint32_t coreMask)
{
   return OSWaitRendezvousWithTimeout(rendezvous, coreMask, 0);
}

BOOL
OSWaitRendezvousWithTimeout(OSRendezvous *rendezvous, uint32_t coreMask, OSTime timeout)
{
   auto core = OSGetCoreId();
   auto success = FALSE;
   auto endTime = timeout ? OSGetTime() + timeout : 0;

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
      if (timeout && OSGetTime() >= endTime) {
         break;
      }
   } while (!success);

   return success;
}

void
CoreInit::registerRendezvousFunctions()
{
   RegisterKernelFunction(OSInitRendezvous);
   RegisterKernelFunction(OSWaitRendezvous);
   RegisterKernelFunction(OSWaitRendezvousWithTimeout);
}
