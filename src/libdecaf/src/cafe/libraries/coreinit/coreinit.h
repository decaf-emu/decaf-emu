#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::coreinit
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::coreinit, "coreinit.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerAlarmSymbols();
   void registerAppIoSymbols();
   void registerAtomicSymbols();
   void registerAtomic64Symbols();
   void registerBspSymbols();
   void registerCacheSymbols();
   void registerClipboardSymbols();
   void registerCodeGenSymbols();
   void registerContextSymbols();
   void registerCoreSymbols();
   void registerCoroutineSymbols();
   void registerCosReportSymbols();
   void registerDeviceSymbols();
   void registerDriverSymbols();
   void registerDynLoadSymbols();
   void registerEventSymbols();
   void registerExceptionSymbols();
   void registerFastMutexSymbols();
   void registerFiberSymbols();
   void registerFsSymbols();
   void registerFsClientSymbols();
   void registerFsCmdSymbols();
   void registerFsCmdBlockSymbols();
   void registerFsDriverSymbols();
   void registerFsStateMachineSymbols();
   void registerFsaSymbols();
   void registerFsaCmdSymbols();
   void registerFsaShimSymbols();
   void registerGhsSymbols();
   void registerHandleSymbols();
   void registerImSymbols();
   void registerInterruptSymbols();
   void registerIosSymbols();
   void registerIpcBufPoolSymbols();
   void registerIpcDriverSymbols();
   void registerLockedCacheSymbols();
   void registerMcpSymbols();
   void registerMemAllocatorSymbols();
   void registerMemBlockHeapSymbols();
   void registerMemDefaultHeapSymbols();
   void registerMemExpHeapSymbols();
   void registerMemFrmHeapSymbols();
   void registerMemHeapSymbols();
   void registerMemListSymbols();
   void registerMemorySymbols();
   void registerMemUnitHeapSymbols();
   void registerMessageQueueSymbols();
   void registerMutexSymbols();
   void registerOsReportSymbols();
   void registerOverlayArenaSymbols();
   void registerRendezvousSymbols();
   void registerSchedulerSymbols();
   void registerScreenSymbols();
   void registerSemaphoreSymbols();
   void registerSnprintfSymbols();
   void registerSpinLockSymbols();
   void registerSystemHeapSymbols();
   void registerSystemInfoSymbols();
   void registerSystemMessageQueueSymbols();
   void registerTaskQueueSymbols();
   void registerThreadSymbols();
   void registerTimeSymbols();
   void registerUserConfigSymbols();
};

} // namespace cafe::coreinit
