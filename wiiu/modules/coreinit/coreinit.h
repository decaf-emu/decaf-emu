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
   void registerFrameHeapFunctions();
   void registerGhsFunctions();
   void registerMembaseFunctions();
   void registerMemoryFunctions();
   void registerMutexFunctions();
   void registerSpinLockFunctions();
   void registerSystemInfoFunctions();
   void registerThreadFunctions();

   void initialiseDynLoad();
   void initialiseGHS();
   void initialiseMembase();
   void initialiseSystemInformation();
};
