#pragma once
#include "kernelmodule.h"

namespace coreinit
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual ~Module() override;

   virtual void initialise() override;

private:
   void initialiseClock();

   void initialiseAlarm();
   void initialiseAtomic64();
   void initialiseDynLoad();
   void initialiseEvent();
   void initialiseGHS();
   void initialiseLockedCache();
   void initialiseMembase();
   void initialiseMessageQueues();
   void initialiseSchedulerFunctions();
   void initialiseShared();
   void initialiseSystemInformation();
   void initialiseUserConfig();

public:
   static void RegisterFunctions();

private:
   static void registerAlarmFunctions();
   static void registerAtomic64Functions();
   static void registerCoreFunctions();
   static void registerCacheFunctions();
   static void registerDebugFunctions();
   static void registerDeviceFunctions();
   static void registerDynLoadFunctions();
   static void registerEventFunctions();
   static void registerExceptionFunctions();
   static void registerExitFunctions();
   static void registerExpHeapFunctions();
   static void registerFastMutexFunctions();
   static void registerFileSystemFunctions();
   static void registerFrameHeapFunctions();
   static void registerGhsFunctions();
   static void registerLockedCacheFunctions();
   static void registerMcpFunctions();
   static void registerMembaseFunctions();
   static void registerMemlistFunctions();
   static void registerMemoryFunctions();
   static void registerMessageQueueFunctions();
   static void registerMutexFunctions();
   static void registerRendezvousFunctions();
   static void registerSchedulerFunctions();
   static void registerSemaphoreFunctions();
   static void registerSharedFunctions();
   static void registerSpinLockFunctions();
   static void registerSystemInfoFunctions();
   static void registerTaskQueueFunctions();
   static void registerThreadFunctions();
   static void registerTimeFunctions();
   static void registerUserConfigFunctions();
   static void registerUnitHeapFunctions();
};

} // namespace coreinit
