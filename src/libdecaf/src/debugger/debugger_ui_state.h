#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{
struct OSThread;
}

namespace debugger
{

namespace ui
{

class StateTracker
{
public:
   //! Returns the current thread that the ui should display info for.
   virtual virt_ptr<cafe::coreinit::OSThread>
   getActiveThread() = 0;

   virtual void
   setActiveThread(virt_ptr<cafe::coreinit::OSThread> thread) = 0;

   virtual unsigned
   getResumeCount() = 0;

   virtual void
   gotoDisassemblyAddress(uint32_t address) = 0;

   virtual void
   gotoMemoryAddress(uint32_t address) = 0;

   virtual void
   gotoStackAddress(uint32_t address) = 0;
};

} // namespace ui

} // namespace debugger
