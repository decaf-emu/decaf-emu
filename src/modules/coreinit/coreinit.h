#pragma once
#include "kernelmodule.h"

class CoreInit : public KernelModuleImpl<CoreInit>
{
public:
   CoreInit();
   virtual ~CoreInit() override;

   virtual void initialise() override;

private:
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
};
