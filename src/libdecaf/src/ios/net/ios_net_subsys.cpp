#include "ios_net_subsys.h"
#include "ios_net_socket.h"

#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_thread.h"

namespace ios::net::internal
{

constexpr auto NetHeapSize = 0x23A000u;
constexpr auto InitThreadStackSize = 0x4000u;
constexpr auto InitThreadPriority = 69u;

using namespace kernel;

struct StaticData
{
   be2_val<HeapId> heap;
   be2_array<uint8_t, NetHeapSize> heapBuffer;
   be2_array<uint8_t, InitThreadStackSize> threadStack;
};

static phys_ptr<StaticData>
sData;

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
   auto error = IOS_CreateHeap(phys_addrof(sData->heapBuffer), NetHeapSize);
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

   return IOS_StartThread(static_cast<ThreadId>(error));
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
   sData = allocProcessStatic<StaticData>();
}

} // namespace ios::net::internal
