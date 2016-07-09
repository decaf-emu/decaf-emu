#include <atomic>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>
#include "cpu.h"

namespace cpu
{

static std::atomic<uint32_t*>
gBreakpoints;

using BreakpointListFn = std::function<uint32_t * (uint32_t*)>;

static inline void
compareAndSwapBreakpoints(uint32_t *bpList, BreakpointListFn functor)
{
   uint32_t *newBpList = nullptr;

   do {
      newBpList = functor(bpList);

      if (newBpList == bpList) {
         // No changes were made
         return;
      }
   } while (!gBreakpoints.compare_exchange_strong(bpList, newBpList));
}

static inline bool
addBreakpoint(uint32_t* bpList, ppcaddr_t address, uint32_t flags)
{
   std::vector<uint32_t> newBpListVec;
   bool bp_changed = true;

   compareAndSwapBreakpoints(bpList, [&](auto bpList) {
      // Reset from any last iteration attempt
      newBpListVec.clear();
      bp_changed = true;

      // Copy all the existing BPs till the terminator
      if (bpList) {
         for (uint32_t *bpListIter = bpList; *bpListIter != 0xFFFFFFFF; ) {
            auto bp_address = *bpListIter++;
            auto bp_flags = *bpListIter++;

            // If its already in the list, lets merge the flags
            if (bp_address == address) {
               // If it already has all the flags, we don't need to change anything
               if ((bp_flags & flags) == flags) {
                  bp_changed = false;
                  return bpList;
               }

               // Flags are additive
               flags |= bp_flags;
               continue;
            }

            newBpListVec.push_back(bp_address);
            newBpListVec.push_back(bp_flags);
         }
      }

      // Add the new breakpoint to the list
      newBpListVec.push_back(address);
      newBpListVec.push_back(flags);

      // Add the terminator to the end
      newBpListVec.push_back(0xFFFFFFFF);

      // Copy to heap memory and return it
      auto newBpList = new uint32_t[newBpListVec.size()];
      memcpy(newBpList, newBpListVec.data(), sizeof(uint32_t) * newBpListVec.size());
      return newBpList;
   });

   return bp_changed;
}

static inline bool
removeBreakpoint(uint32_t *bpList,
                 ppcaddr_t address,
                 uint32_t flags)
{
   std::vector<uint32_t> newBpListVec;
   bool bp_matched = false;

   compareAndSwapBreakpoints(bpList, [&](auto bpList) {
      // If there is no existing list, we have nothing to do
      if (!bpList) {
         return bpList;
      }

      // Reset from any last iteration attempt
      newBpListVec.clear();
      bp_matched = false;

      // Copy all the data which has the system flag, otherwise ignore it
      for (uint32_t *bpListIter = bpList; *bpListIter != 0xFFFFFFFF; ) {
         auto bp_address = *bpListIter++;
         auto bp_flags = *bpListIter++;

         if (bp_address == address) {
            // If it has none of the flags, we have no changes to make
            if ((bp_flags & flags) == 0) {
               return bpList;
            }

            // If it has every flag, switch the return value
            if ((bp_flags & flags) == flags) {
               bp_matched = true;
            }

            // Flags are subtractive
            bp_flags &= ~flags;
         }

         // Only put the breakpoint back in the list if it has flags left
         if (bp_flags) {
            newBpListVec.push_back(bp_address);
            newBpListVec.push_back(bp_flags);
         }
      }

      // If we have no breakpoints, just return no list
      if (newBpListVec.size() == 0) {
         return static_cast<uint32_t*>(nullptr);
      }

      // Add the terminator to the end
      newBpListVec.push_back(0xFFFFFFFF);

      // Copy to heap memory and return it
      auto newBpList = new uint32_t[newBpListVec.size()];
      memcpy(newBpList, newBpListVec.data(), sizeof(uint32_t) * newBpListVec.size());
      return newBpList;
   });

   return bp_matched;
}

static inline bool
clearBreakpoints(uint32_t *bpList,
                 uint32_t flags_mask)
{
   std::vector<uint32_t> newBpListVec;
   bool bps_changed = false;

   compareAndSwapBreakpoints(bpList, [&](auto bpList) {
      // If there is no existing list, we have nothing to do
      if (!bpList) {
         return bpList;
      }

      // Reset from any last iteration attempt
      bps_changed = false;
      newBpListVec.clear();

      // Copy all the data which has the system flag, otherwise ignore it
      for (uint32_t *bpListIter = bpList; *bpListIter != 0xFFFFFFFF; ) {
         auto bp_address = *bpListIter++;
         auto bp_flags = *bpListIter++;

         // If it has any of these flags, clear them
         if (bp_flags & flags_mask) {
            bp_flags &= ~flags_mask;
            bps_changed = true;
         }

         // If it still has flags, put it back on the list
         if (bp_flags) {
            newBpListVec.push_back(bp_address);
            newBpListVec.push_back(bp_flags);
         }
      }

      // If we have no changes, dont update
      if (!bps_changed) {
         return bpList;
      }

      // If we have no breakpoints, just return no list
      if (newBpListVec.size() == 0) {
         return static_cast<uint32_t*>(nullptr);
      }

      // Add the terminator to the end
      newBpListVec.push_back(0xFFFFFFFF);

      // Copy to heap memory and return it
      auto newBpList = new uint32_t[newBpListVec.size()];
      memcpy(newBpList, newBpListVec.data(), sizeof(uint32_t) * newBpListVec.size());
      return newBpList;
   });

   return bps_changed;
}

bool
hasBreakpoints()
{
   return !!gBreakpoints.load(std::memory_order_acquire);
}

bool
popBreakpoint(ppcaddr_t address)
{
   auto bpList = gBreakpoints.load(std::memory_order_acquire);

   if (bpList == nullptr) {
      // No breakpoints
      return false;
   }

   auto flags = 0u;

   for (auto *bpListIter = bpList; *bpListIter != 0xFFFFFFFF; ) {
      auto bp_address = *bpListIter++;
      auto bp_flags = *bpListIter++;

      if (bp_address == address) {
         flags = bp_flags;
         break;
      }
   }

   // Short circuit normal flags
   if (!flags) {
      return false;
   } else if (!(flags & SYSTEM_BPFLAG)) {
      return true;
   }

   // We have a system flag, which are one-hit this means we need to remove it
   // remove_breakpoint will return false if it fails to find and remove that
   // breakpoint.  We have no need to loop as we are guarenteed not to find it
   // by looping again since we only pass remove_breakpoint one flag.
   return removeBreakpoint(bpList, address, SYSTEM_BPFLAG);
}

bool
clearBreakpoints(uint32_t flags_mask)
{
   auto bpList = gBreakpoints.load(std::memory_order_acquire);
   return clearBreakpoints(bpList, flags_mask);
}

bool
addBreakpoint(ppcaddr_t address,
              uint32_t flags)
{
   if (address == 0xFFFFFFFF) {
      // Cannot use this address as it is the terminator
      throw std::logic_error("0xFFFFFFFF is not a valid address to set a breakpoint on");
   }

   if (!flags) {
      throw std::logic_error("You must specify at least a single flag for a breakpoint");
   }

   auto bpList = gBreakpoints.load(std::memory_order_acquire);
   return addBreakpoint(bpList, address, flags);
}

bool
removeBreakpoint(ppcaddr_t address,
                 uint32_t flags)
{
   auto bpList = gBreakpoints.load(std::memory_order_acquire);
   return removeBreakpoint(bpList, address, flags);
}

} // namespace cpu
