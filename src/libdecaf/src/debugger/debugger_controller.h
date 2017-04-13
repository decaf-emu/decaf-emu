#pragma once
#include "debugger_interface.h"
#include <atomic>
#include <array>
#include <condition_variable>
#include <mutex>
#include <string>

namespace debugger
{

class Controller : public DebuggerInterface
{
public:
   Controller()
   {
   }

   void onDebugBreakInterrupt();

   virtual bool paused() override;
   virtual cpu::Core *getPauseContext(unsigned core) override;
   virtual unsigned getPauseInitiator() override;

   virtual void pause() override;
   virtual void resume() override;
   virtual void stepInto(unsigned core) override;
   virtual void stepOver(unsigned core) override;

   virtual bool hasBreakpoint(uint32_t address) override;
   virtual void addBreakpoint(uint32_t address) override;
   virtual void removeBreakpoint(uint32_t address) override;

private:
   bool mEnabled = false;

   //! Whether we are currently paused.
   std::atomic_bool mIsPaused;

   //! Used to synchronise cores across a pause.
   std::mutex mPauseMutex;

   //! Used to synchronise cores across a pause.
   std::condition_variable mPauseReleaseCond;

   //! The context running on each core at the time of a pause.
   std::array<cpu::Core *, 3> mPausedContexts;

   //! Which core initiated the pause by sending a DbgBreak interrupt.
   unsigned mPauseInitiator;

   //! Which cores are trying to pause.
   std::atomic<unsigned> mCoresPausing;

   //! Which cores are trying to resume.
   std::atomic<unsigned> mCoresResuming;
};

} // namespace debugger
