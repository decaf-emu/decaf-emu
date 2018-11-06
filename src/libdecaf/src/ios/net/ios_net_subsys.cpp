#include "ios_net_subsys.h"
#include "ios_net_socket_thread.h"

#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_thread.h"

namespace ios::net::internal
{

constexpr auto SubsysHeapSize = 0x23A000u;
constexpr auto InitThreadStackSize = 0x4000u;
constexpr auto InitThreadPriority = 69u;

using namespace kernel;

struct StaticSubsysData
{
   be2_val<HeapId> heap;
   be2_array<uint8_t, InitThreadStackSize> threadStack;
};

static phys_ptr<StaticSubsysData>
sData;

static phys_ptr<void>
sSubsysHeapBuffer;

static Error
subsysInitThread(phys_ptr<void> /*context*/)
{
   // Read network config

   // Start ifmgr thread

   // Start socket thread
   auto error = startSocketThread();
   if (error < Error::OK) {
      return error;
   }

   // Start ifnet thread

   // Create some weird timer thread

   return Error::OK;
}

Error
initSubsys()
{
   // Init ifmgr resource manager

   // Init socket resource manager
   auto error = registerSocketResourceManager();
   if (error < Error::OK) {
      return error;
   }

   // Init ifnet resource manager
   return Error::OK;
}

Error
startSubsys()
{
   auto error = IOS_CreateHeap(sSubsysHeapBuffer, SubsysHeapSize);
   if (error < Error::OK) {
      return error;
   }

   error = IOS_CreateThread(&subsysInitThread,
                            nullptr,
                            phys_addrof(sData->threadStack) + sData->threadStack.size(),
                            sData->threadStack.size(),
                            InitThreadPriority,
                            ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   auto threadId = static_cast<ThreadId>(error);
   kernel::internal::setThreadName(threadId, "NetSubsysThread");
   return IOS_StartThread(threadId);
}

Error
stopSubsys()
{
   stopSocketThread();
   return Error::OK;
}

void
initialiseStaticSubsysData()
{
   sData = allocProcessStatic<StaticSubsysData>();
   sSubsysHeapBuffer = allocProcessLocalHeap(SubsysHeapSize);
}

} // namespace ios::net::internal
