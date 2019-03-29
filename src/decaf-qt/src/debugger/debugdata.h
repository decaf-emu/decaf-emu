#pragma once
#include <libcpu/jit_stats.h>
#include <libdecaf/decaf_debug_api.h>
#include <vector>
#include <QObject>

class DebugData : public QObject
{
   Q_OBJECT

public:
   using AnalyseDatabase = decaf::debug::AnalyseDatabase;
   using CafeThread = decaf::debug::CafeThread;
   using CafeMemorySegment = decaf::debug::CafeMemorySegment;
   using CafeModuleInfo = decaf::debug::CafeModuleInfo;
   using CafeVoice = decaf::debug::CafeVoice;
   using JitStats = cpu::jit::JitStats;
   using Pm4CaptureState = decaf::debug::Pm4CaptureState;
   using VirtualAddress = decaf::debug::VirtualAddress;

   DebugData(QObject *parent = nullptr) :
      QObject(parent)
   {
   }

   int activeThreadIndex()
   {
      return mActiveThreadIndex;
   }

   const CafeThread *activeThread()
   {
      if (mActiveThreadIndex < 0 || mActiveThreadIndex >= mThreads.size()) {
         return nullptr;
      }

      return &mThreads[mActiveThreadIndex];
   }

   const CafeModuleInfo &loadedModule()
   {
      return mLoadedModule;
   }

   const AnalyseDatabase &analyseDatabase()
   {
      return mAnalyseDatabase;
   }

   const JitStats &jitStats()
   {
      return mJitStats;
   }

   const std::vector<CafeThread> &threads()
   {
      return mThreads;
   }

   const std::vector<CafeMemorySegment> &segments()
   {
      return mSegments;
   }

   const std::vector<CafeVoice> &voices()
   {
      return mVoices;
   }

   CafeMemorySegment *segmentForAddress(VirtualAddress address)
   {
      for (auto &segment : mSegments) {
         if (address >= segment.address && address - segment.address < segment.size) {
            return &segment;
         }
      }

      return nullptr;
   }

   bool paused()
   {
      return mPaused;
   }

   bool update();
   void setActiveThreadIndex(int index);

   void setJitProfilingState(bool core0, bool core1, bool core2);

signals:
   void entry();
   void dataChanged();
   void activeThreadIndexChanged();

   void pm4CaptureStateChanged(Pm4CaptureState state);
   void executionStateChanged(bool paused);
   void jitProfilingStateChanged(bool core0, bool core1, bool core2);

private:
   bool mEntryHit = false;
   int mActiveThreadIndex = -1;

   AnalyseDatabase mAnalyseDatabase = { };
   CafeModuleInfo mLoadedModule = { };

   std::vector<CafeThread> mThreads;
   std::vector<CafeMemorySegment> mSegments;
   std::vector<CafeVoice> mVoices;

   cpu::jit::JitStats mJitStats = { };

   bool mPaused = false;
   Pm4CaptureState mPm4CaptureState = Pm4CaptureState::Disabled;

   unsigned mJitProfilingMask = 0u;
};
