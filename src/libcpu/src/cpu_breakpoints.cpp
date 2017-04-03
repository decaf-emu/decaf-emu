#include "cpu.h"
#include "cpu_breakpoints.h"
#include "espresso/espresso_instructionset.h"
#include "mem.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace cpu
{

static std::shared_ptr<BreakpointList>
sActiveBreakpoints;

using ModifyListFn = std::function<bool (BreakpointList &list)>;

static inline void
updateBreakpointList(ModifyListFn fn)
{
   auto newList = std::make_shared<BreakpointList>();
   auto currentList = sActiveBreakpoints;

   do {
      if (currentList) {
         *newList = *currentList;
      } else {
         newList->clear();
      }

      if (!fn(*newList)) {
         // If function returns false, do not update breakpoint list.
         break;
      }
   } while (!std::atomic_compare_exchange_strong(&sActiveBreakpoints, &currentList, newList));
}


/**
 * Add a breakpoint at an address.
 */
void
addBreakpoint(uint32_t address,
              Breakpoint::Type type)
{
   auto savedCode = mem::read<uint32_t>(address);

   updateBreakpointList([address, type, savedCode](BreakpointList &list) {
      auto itr = std::find_if(list.begin(), list.end(),
                              [address](auto &bp) {
                                 return bp.address == address;
                              });

      if (itr != list.end()) {
         if (itr->type == type) {
            // Same type of breakpoint already at address, do not modify list.
            return false;
         }

         if (type == Breakpoint::SingleFire && itr->type == Breakpoint::MultiFire) {
            // Already have a multi fire breakpoint, no need to set single fire.
            return false;
         }

         // Update existing breakpoint type.
         itr->type = type;
         return true;
      }

      list.push_back({ type, address, savedCode });
      return true;
   });

   // Set unconditional trap instruction at address.
   auto trapInstr = espresso::encodeInstruction(espresso::InstructionID::tw);
   trapInstr.to = 31;
   trapInstr.rA = 0;
   trapInstr.rB = 0;
   mem::write<uint32_t>(address, trapInstr);

   cpu::invalidateInstructionCache(address, 4);
}


/**
 * Remove a breakpoint at an address.
 */
void
removeBreakpoint(uint32_t address)
{
   updateBreakpointList([address](BreakpointList &list) {
      auto itr = std::find_if(list.begin(), list.end(),
                              [address](auto &bp) {
                                 return bp.address == address;
                              });

      if (itr == list.end()) {
         return false;
      }

      // Restore saved code.
      mem::write<uint32_t>(address, itr->savedCode);

      list.erase(itr);
      return true;
   });

   cpu::invalidateInstructionCache(address, 4);
}


/**
 * Returns true if we hit a breakpoint at the address.
 *
 * Will remove the breakpoint if it is a SingleFire breakpoint.
 */
bool
testBreakpoint(uint32_t address)
{
   auto list = sActiveBreakpoints;

   if (!list) {
      return false;
   }

   auto itr = std::find_if(list->begin(), list->end(),
                           [address](auto &bp) {
                              return bp.address == address;
                           });

   if (itr == list->end()) {
      return false;
   }

   if (itr->type == Breakpoint::SingleFire) {
      removeBreakpoint(address);
   }

   return true;
}


/**
 * Returns true if there are any breakpoints set.
 */
bool
hasBreakpoints()
{
   return sActiveBreakpoints != nullptr;
}


/**
 * Returns true if there is a breakpoint at the specified address.
 */
bool
hasBreakpoint(uint32_t address)
{
   auto list = sActiveBreakpoints;

   if (!list) {
      return false;
   }

   auto itr = std::find_if(list->begin(), list->end(),
                           [address](auto &bp) {
                              return bp.address == address;
                           });

   return itr != list->end();
}


/**
 * Get a list of all active breakpoints.
 */
std::shared_ptr<BreakpointList>
getBreakpoints()
{
   return sActiveBreakpoints;
}


/**
 * Get the instruction at an address before we edited it with a tw.
 *
 * If there is no breakpoint, reads the memory and returns the current instruction.
 */
uint32_t
getBreakpointSavedCode(uint32_t address)
{
   auto list = sActiveBreakpoints;
   if (list) {
      auto itr = std::find_if(list->begin(), list->end(),
                              [address](auto &bp) {
                                 return bp.address == address;
                              });

      if (itr != list->end()) {
         return itr->savedCode;
      }
   }

   return mem::read<uint32_t>(address);
}

} // namespace cpu
