#pragma once
#include "address.h"
#include <common/decaf_assert.h>
#include "mmu.h"

namespace cpu
{

namespace internal
{

void
initialiseMemtrack();

void
registerTrackedRange(VirtualAddress virtualAddress,
                     PhysicalAddress physicalAddress,
                     uint32_t size);

void
unregisterTrackedRange(VirtualAddress virtualAddress,
                       uint32_t size);

void
clearTrackedRanges();

} // namespace internal

struct MemtrackState
{
   bool operator==(const MemtrackState& other) const
   {
      return state == other.state;
   }

   bool operator!=(const MemtrackState& other) const
   {
      return state != other.state;
   }

   uint64_t state;
};

MemtrackState
getMemoryState(PhysicalAddress physicalAddress,
               uint32_t size);

} // namespace cpu
