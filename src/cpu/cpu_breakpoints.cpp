#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include "cpu.h"

namespace cpu
{

static std::shared_ptr<uint32_t>
   gBreakpoints;

static inline void compare_and_swap_breakpoints(std::shared_ptr<uint32_t>& bpListPtr, std::function<uint32_t*(uint32_t*)> functor)
{
   std::shared_ptr<uint32_t> newBpListPtr;
   do {
      auto bpList = bpListPtr.get();
      auto newBpList = functor(bpList);
      if (newBpList == bpList) {
         // No changes were made
         return;
      } else if (newBpList) {
         // We got a new list
         newBpListPtr = std::shared_ptr<uint32_t>(newBpList, std::default_delete<uint32_t[]>());
      } else {
         // We are swapping to an empty list
         newBpListPtr = std::shared_ptr<uint32_t>();
      }
   } while (!std::atomic_compare_exchange_weak(&gBreakpoints, &bpListPtr, newBpListPtr));
}

static inline bool add_breakpoint(std::shared_ptr<uint32_t>& bpListPtr, ppcaddr_t address, uint32_t flags)
{
   std::vector<uint32_t> newBpListVec;
   bool bp_changed = true;
   compare_and_swap_breakpoints(bpListPtr, [&](auto bpList) {
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

static inline bool remove_breakpoint(std::shared_ptr<uint32_t>& bpListPtr, ppcaddr_t address, uint32_t flags)
{
   std::vector<uint32_t> newBpListVec;
   bool bp_matched = false;
   compare_and_swap_breakpoints(bpListPtr, [&](auto bpList) {
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

static inline bool clear_breakpoints(std::shared_ptr<uint32_t>& bpListPtr, uint32_t flags_mask)
{
   std::vector<uint32_t> newBpListVec;
   bool bps_changed = false;
   compare_and_swap_breakpoints(bpListPtr, [&](auto bpList) {
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

bool has_breakpoints()
{
   auto bpListPtr = std::atomic_load(&gBreakpoints);
   return bpListPtr.get() != nullptr;
}

bool pop_breakpoint(ppcaddr_t address)
{
   auto bpListPtr = std::atomic_load(&gBreakpoints);

   auto bpList = bpListPtr.get();
   if (bpList == nullptr) {
      // No breakpoints
      return false;
   }

   uint32_t found_flags = 0;
   for (uint32_t *bpListIter = bpList; *bpListIter != 0xFFFFFFFF; ) {
      auto bp_address = *bpListIter++;
      auto bp_flags = *bpListIter++;
      if (bp_address == address) {
         found_flags = bp_flags;
         break;
      }
   }

   // Short circuit normal flags
   if (!found_flags) {
      return false;
   } else if (!(found_flags & SYSTEM_BPFLAG)) {
      return true;
   }

   // We have a system flag, which are one-hit this means we need to remove it
   // remove_breakpoint will return false if it fails to find and remove that
   // breakpoint.  We have no need to loop as we are guarenteed not to find it
   // by looping again since we only pass remove_breakpoint one flag.
   return remove_breakpoint(bpListPtr, address, SYSTEM_BPFLAG);
}

bool clear_breakpoints(uint32_t flags_mask)
{
   auto bpListPtr = std::atomic_load(&gBreakpoints);
   return clear_breakpoints(bpListPtr, flags_mask);
}

bool add_breakpoint(ppcaddr_t address, uint32_t flags)
{
   if (address == 0xFFFFFFFF) {
      // Cannot use this address as it is the terminator
      throw std::logic_error("0xFFFFFFFF is not a valid address to set a breakpoint on");
   }
   if (!flags) {
      throw std::logic_error("You must specify at least a single flag for a breakpoint");
   }

   auto bpListPtr = std::atomic_load(&gBreakpoints);
   return add_breakpoint(bpListPtr, address, flags);
}

bool remove_breakpoint(ppcaddr_t address, uint32_t flags)
{
   auto bpListPtr = std::atomic_load(&gBreakpoints);
   return remove_breakpoint(bpListPtr, address, flags);
}

} // namespace cpu
