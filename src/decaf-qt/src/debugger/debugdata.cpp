#include "debugdata.h"

#include <libcpu/cpu_breakpoints.h>
#include <libcpu/jit_stats.h>
#include <libdecaf/decaf_debug_api.h>

bool
DebugData::update()
{
   if (!decaf::debug::ready()) {
      return false;
   }

   mThreads.clear();
   mSegments.clear();
   mVoices.clear();

   decaf::debug::sampleCafeThreads(mThreads);
   decaf::debug::sampleCafeMemorySegments(mSegments);
   decaf::debug::sampleCafeVoices(mVoices);
   decaf::debug::sampleCpuBreakpoints(mBreakpoints);
   cpu::jit::sampleStats(mJitStats);

   if (!mEntryHit) {
      if (decaf::debug::getLoadedModuleInfo(mLoadedModule)) {
         // TODO: Spawn thread to do analysis
         decaf::debug::analyseLoadedModules(mAnalyseDatabase);
         decaf::debug::analyseCode(mAnalyseDatabase,
                                   mLoadedModule.textAddr,
                                   mLoadedModule.textAddr + mLoadedModule.textSize);
      }

      mEntryHit = true;
      emit entry();
   }

   auto paused = decaf::debug::isPaused();
   auto pauseInitiatorCoreId = 0;
   auto pauseNia = VirtualAddress { 0 };
   if (paused) {
      pauseInitiatorCoreId = decaf::debug::getPauseInitiatorCoreId();
      pauseNia = decaf::debug::getPausedContext(pauseInitiatorCoreId)->nia;
   }

   if ((paused != mPaused) ||
       (pauseInitiatorCoreId != mPauseInitiatorCoreId) ||
       (pauseNia != mPauseNia)) {
      mPaused = paused;
      mPauseNia = pauseNia;
      mPauseInitiatorCoreId = pauseInitiatorCoreId;
      emit executionStateChanged(paused, pauseInitiatorCoreId, pauseNia);
   }

   auto captureState = decaf::debug::pm4CaptureState();
   if (captureState != mPm4CaptureState) {
      mPm4CaptureState = captureState;
      emit pm4CaptureStateChanged(captureState);
   }

   auto jitProfilingMask = cpu::jit::getProfilingMask();
   if (jitProfilingMask != mJitProfilingMask) {
      mJitProfilingMask = jitProfilingMask;
      emit jitProfilingStateChanged(
         jitProfilingMask & (1 << 0),
         jitProfilingMask & (1 << 1),
         jitProfilingMask & (1 << 2));
   }

   emit dataChanged();
   return true;
}

void
DebugData::setActiveThreadIndex(int index)
{
   mActiveThreadIndex = index;
   emit activeThreadIndexChanged();
}

void
DebugData::setJitProfilingState(bool core0, bool core1, bool core2)
{
   cpu::jit::setProfilingMask(((core0 ? 1 : 0) << 0) |
                              ((core1 ? 1 : 0) << 1) |
                              ((core2 ? 1 : 0) << 2));
}
