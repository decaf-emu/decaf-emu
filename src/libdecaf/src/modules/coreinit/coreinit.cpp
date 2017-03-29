#include "coreinit.h"
#include "coreinit_fsa.h"

namespace coreinit
{

Module::~Module()
{
}

void
Module::initialise()
{
   // Always initialise clock first
   initialiseClock();

   initialiseAlarm();
   initialiseAllocatorFunctions();
   initialiseEvent();
   initialiseExceptions();
   initialiseGHS();
   initialiseGhsTypeInfo();
   initialiseLockedCache();
   initialiseMembase();
   initialiseMessageQueues();
   initialiseSchedulerFunctions();
   initialiseShared();
   initialiseSystemInformation();
   initialiseUserConfig();
}

void
Module::RegisterFunctions()
{
   registerAlarmFunctions();
   registerAppIoFunctions();
   registerAllocatorFunctions();
   registerAtomic64Functions();
   registerBlockHeapFunctions();
   registerCoreFunctions();
   registerCoroutineFunctions();
   registerCacheFunctions();
   registerDebugFunctions();
   registerDeviceFunctions();
   registerDriverFunctions();
   registerDynLoadFunctions();
   registerEventFunctions();
   registerExceptionFunctions();
   registerExitFunctions();
   registerExpHeapFunctions();
   registerFastMutexFunctions();
   registerFsFunctions();
   registerFsCmdFunctions();
   registerFsCmdBlockFunctions();
   registerFsClientFunctions();
   registerFsDriverFunctions();
   registerFsFsmFunctions();

   // coreinit_fsa
   RegisterKernelFunction(FSAGetAsyncResult);

   registerFsaShimFunctions();
   registerFrameHeapFunctions();
   registerGhsFunctions();
   registerGhsTypeInfoFunctions();
   registerImFunctions();
   registerInterruptFunctions();
   registerIosFunctions();
   registerIpcFunctions();
   registerLockedCacheFunctions();
   registerMcpFunctions();
   registerMemoryFunctions();
   registerMembaseFunctions();
   registerMemlistFunctions();
   registerMessageQueueFunctions();
   registerMutexFunctions();
   registerOverlayArenaFunctions();
   registerRendezvousFunctions();
   registerSchedulerFunctions();
   registerScreenFunctions();
   registerSemaphoreFunctions();
   registerSharedFunctions();
   registerSpinLockFunctions();
   registerSprintfFunctions();
   registerSystemInfoFunctions();
   registerTaskQueueFunctions();
   registerThreadFunctions();
   registerTimeFunctions();
   registerUserConfigFunctions();
   registerUnitHeapFunctions();
}

} // namespace coreinit
