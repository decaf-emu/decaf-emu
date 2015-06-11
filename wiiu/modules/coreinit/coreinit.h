#pragma once
#include "systemmodule.h"
#include "log.h"

class CoreInit : public SystemModule
{
public:
   CoreInit();

   virtual void initialise() override;

private:
   void registerCacheFunctions();
   void registerDebugFunctions();
   void registerDynLoadFunctions();
   void registerExpHeapFunctions();
   void registerFileSystemFunctions();
   void registerFrameHeapFunctions();
   void registerGhsFunctions();
   void registerMcpFunctions();
   void registerMembaseFunctions();
   void registerMemoryFunctions();
   void registerMessageQueueFunctions();
   void registerMutexFunctions();
   void registerSaveFunctions();
   void registerSpinLockFunctions();
   void registerSystemInfoFunctions();
   void registerThreadFunctions();
   void registerTimeFunctions();
   void registerUserConfigFunctions();

   void initialiseDynLoad();
   void initialiseGHS();
   void initialiseMembase();
   void initialiseMessageQueues();
   void initialiseSystemInformation();
};
