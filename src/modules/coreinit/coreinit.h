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
   void initialiseDynLoad();
   void initialiseEvent();
   void initialiseGHS();
   void initialiseMembase();
   void initialiseMessageQueues();
   void initialiseSchedulerFunctions();
   void initialiseShared();
   void initialiseSystemInformation();
   void initialiseThread();

public:
   static void RegisterFunctions();

private:
   static void registerAlarmFunctions();
   static void registerCoreFunctions();
   static void registerCacheFunctions();
   static void registerDebugFunctions();
   static void registerDeviceFunctions();
   static void registerDynLoadFunctions();
   static void registerEventFunctions();
   static void registerExceptionFunctions();
   static void registerExpHeapFunctions();
   static void registerFastMutexFunctions();
   static void registerFileSystemFunctions();
   static void registerFrameHeapFunctions();
   static void registerGhsFunctions();
   static void registerMcpFunctions();
   static void registerMembaseFunctions();
   static void registerMemlistFunctions();
   static void registerMemoryFunctions();
   static void registerMessageQueueFunctions();
   static void registerMutexFunctions();
   static void registerSchedulerFunctions();
   static void registerSemaphoreFunctions();
   static void registerSharedFunctions();
   static void registerSpinLockFunctions();
   static void registerSystemInfoFunctions();
   static void registerThreadFunctions();
   static void registerTimeFunctions();
   static void registerUserConfigFunctions();
};
