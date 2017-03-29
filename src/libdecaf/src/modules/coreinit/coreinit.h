#pragma once
#include "kernel/kernel_hlemodule.h"

/**
 * \defgroup coreinit coreinit
 *
 * Contains all core operating system functions such as threads, synchronisation
 * objects, filesystem, memory, exception handling, etc...
 */

namespace coreinit
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual ~Module() override;

   virtual void initialise() override;

private:
   void initialiseClock();

   void initialiseAlarm();
   void initialiseAllocatorFunctions();
   void initialiseEvent();
   void initialiseExceptions();
   void initialiseGHS();
   void initialiseGhsTypeInfo();
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
   static void registerAppIoFunctions();
   static void registerAllocatorFunctions();
   static void registerAtomic64Functions();
   static void registerBlockHeapFunctions();
   static void registerCoreFunctions();
   static void registerCoroutineFunctions();
   static void registerCacheFunctions();
   static void registerDebugFunctions();
   static void registerDeviceFunctions();
   static void registerDriverFunctions();
   static void registerDynLoadFunctions();
   static void registerEventFunctions();
   static void registerExceptionFunctions();
   static void registerExitFunctions();
   static void registerExpHeapFunctions();
   static void registerFastMutexFunctions();
   static void registerFsFunctions();
   static void registerFsCmdFunctions();
   static void registerFsCmdBlockFunctions();
   static void registerFsClientFunctions();
   static void registerFsDriverFunctions();
   static void registerFsFsmFunctions();
   static void registerFsaShimFunctions();
   static void registerFrameHeapFunctions();
   static void registerGhsFunctions();
   static void registerGhsTypeInfoFunctions();
   static void registerImFunctions();
   static void registerInterruptFunctions();
   static void registerIosFunctions();
   static void registerIpcFunctions();
   static void registerLockedCacheFunctions();
   static void registerMcpFunctions();
   static void registerMembaseFunctions();
   static void registerMemlistFunctions();
   static void registerMemoryFunctions();
   static void registerMessageQueueFunctions();
   static void registerMutexFunctions();
   static void registerOverlayArenaFunctions();
   static void registerRendezvousFunctions();
   static void registerSchedulerFunctions();
   static void registerScreenFunctions();
   static void registerSemaphoreFunctions();
   static void registerSharedFunctions();
   static void registerSpinLockFunctions();
   static void registerSprintfFunctions();
   static void registerSystemInfoFunctions();
   static void registerTaskQueueFunctions();
   static void registerThreadFunctions();
   static void registerTimeFunctions();
   static void registerUserConfigFunctions();
   static void registerUnitHeapFunctions();
};

} // namespace coreinit
