#pragma once
#include "systemmodule.h"
#include "log.h"

class CoreInit : public SystemModuleImpl<CoreInit>
{
public:
   CoreInit();
   virtual ~CoreInit() override;

   virtual void initialise() override;

private:
   void initialiseDynLoad();
   void initialiseGHS();
   void initialiseMembase();
   void initialiseMessageQueues();
   void initialiseSystemInformation();

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
   static void registerCacheFunctions();
   static void registerDebugFunctions();
   static void registerDeviceFunctions();
   static void registerDynLoadFunctions();
   static void registerEventFunctions();
   static void registerExpHeapFunctions();
   static void registerFastMutexFunctions();
   static void registerFileSystemFunctions();
   static void registerFrameHeapFunctions();
   static void registerGhsFunctions();
   static void registerMcpFunctions();
   static void registerMembaseFunctions();
   static void registerMemoryFunctions();
   static void registerMessageQueueFunctions();
   static void registerMutexFunctions();
   static void registerSpinLockFunctions();
   static void registerSystemInfoFunctions();
   static void registerThreadFunctions();
   static void registerTimeFunctions();
   static void registerUserConfigFunctions();
};
