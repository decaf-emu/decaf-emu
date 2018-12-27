#pragma once
#include <libdecaf/decaf_debug_api.h>

namespace debugui
{

using CafeThread = decaf::debug::CafeThread;

class StateTracker
{
public:
   //! Returns the current thread that the ui should display info for.
   virtual const CafeThread &
   getActiveThread() = 0;

   virtual void
   setActiveThread(const CafeThread &thread) = 0;

   virtual unsigned
   getResumeCount() = 0;

   virtual void
   gotoDisassemblyAddress(uint32_t address) = 0;

   virtual void
   gotoMemoryAddress(uint32_t address) = 0;

   virtual void
   gotoStackAddress(uint32_t address) = 0;
};

} // namespace debugui
