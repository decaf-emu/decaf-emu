#include "debugdata.h"
#include <libcpu/jit_stats.h>

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
   if (paused != mPaused) {
      mPaused = paused;
      emit executionStateChanged(paused);
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
