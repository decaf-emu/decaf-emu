#pragma once
#include <libcpu/cpu_breakpoints.h>
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
   using CpuBreakpoint = decaf::debug::CpuBreakpoint;
   using JitStats = cpu::jit::JitStats;
   using Pm4CaptureState = decaf::debug::Pm4CaptureState;
   using VirtualAddress = decaf::debug::VirtualAddress;

   DebugData(QObject *parent = nullptr) :
      QObject(parent)
   {
   }

   int activeThreadIndex() const
   {
      return mActiveThreadIndex;
   }

   const CafeThread *activeThread() const
   {
      if (mActiveThreadIndex < 0 || mActiveThreadIndex >= mThreads.size()) {
         return nullptr;
      }

      return &mThreads[mActiveThreadIndex];
   }

   const CafeModuleInfo &loadedModule() const
   {
      return mLoadedModule;
   }

   const AnalyseDatabase &analyseDatabase() const
   {
      return mAnalyseDatabase;
   }

   const JitStats &jitStats() const
   {
      return mJitStats;
   }

   const std::vector<CpuBreakpoint> breakpoints() const
   {
      return mBreakpoints;
   }

   const CpuBreakpoint *getBreakpoint(VirtualAddress address) const
   {
      for (auto &breakpoint : mBreakpoints) {
         if (breakpoint.address == address) {
            return &breakpoint;
         }
      }

      return nullptr;
   }

   const std::vector<CafeThread> &threads() const
   {
      return mThreads;
   }

   const std::vector<CafeMemorySegment> &segments() const
   {
      return mSegments;
   }

   const std::vector<CafeVoice> &voices() const
   {
      return mVoices;
   }

   const CafeMemorySegment *segmentForAddress(VirtualAddress address) const
   {
      for (auto &segment : mSegments) {
         if (address >= segment.address && address - segment.address < segment.size) {
            return &segment;
         }
      }

      return nullptr;
   }

   bool paused() const
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
   void executionStateChanged(bool paused, int pauseInitiatorCoreId,
                              VirtualAddress pauseNia);
   void jitProfilingStateChanged(bool core0, bool core1, bool core2);

private:
   bool mEntryHit = false;
   int mActiveThreadIndex = -1;

   AnalyseDatabase mAnalyseDatabase = { };
   CafeModuleInfo mLoadedModule = { };

   std::vector<CafeThread> mThreads;
   std::vector<CafeMemorySegment> mSegments;
   std::vector<CafeVoice> mVoices;

   std::vector<CpuBreakpoint> mBreakpoints;
   cpu::jit::JitStats mJitStats = { };

   bool mPaused = false;
   int mPauseInitiatorCoreId = -1;
   VirtualAddress mPauseNia = 0u;

   Pm4CaptureState mPm4CaptureState = Pm4CaptureState::Disabled;

   unsigned mJitProfilingMask = 0u;
};
