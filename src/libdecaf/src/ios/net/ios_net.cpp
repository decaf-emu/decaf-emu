#include "ios_net.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios/ios_stackobject.h"

#include <common/log.h>

namespace ios::net
{

using namespace kernel;

constexpr auto LocalHeapSize = 0x40000u;
constexpr auto CrossHeapSize = 0xC0000u;

struct StaticData
{
   be2_array<std::byte, LocalHeapSize> localHeapBuffer;
};

static phys_ptr<StaticData>
sData = nullptr;

namespace internal
{

static void
initialiseStaticData()
{
   sData = kernel::allocProcessStatic<StaticData>();
}

} // namespace internal

Error
processEntryPoint(phys_ptr<void> /* context */)
{
   constexpr auto NumMessages = 10u;
   StackArray<Message, NumMessages> messageBuffer;

   // Initialise static memory
   internal::initialiseStaticData();

   // Initialise process heaps
   auto error = IOS_CreateLocalProcessHeap(phys_addrof(sData->localHeapBuffer),
                                                   static_cast<uint32_t>(sData->localHeapBuffer.size()));
   if (error < Error::OK) {
      gLog->error("NET: Failed to create local process heap, error = {}.", error);
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      gLog->error("NET: Failed to create cross process heap, error = {}.", error);
      return error;
   }

   // TODO: bspGetClockInfo
   // TODO: initIoBuf
   // TODO: startWifi24Thread (/dev/wifi24)
   // TODO: startUdsThreads (/dev/ifuds /dev/udscntrl)
   // TODO: startSomeThreads (/dev/ac_main /dev/ndm /dev/dlp)

   // TODO: init subsys (/dev/ifmgr /dev/socket /dev/ifnet)

   error = IOS_CreateMessageQueue(messageBuffer, NumMessages);
   if (error < Error::OK) {
      gLog->error("NET: Failed to create message queue, error = {}.", error);
      return error;
   }

   auto messageQueueId = static_cast<MessageQueueId>(error);

   return Error::OK;
}

} // namespace ios::net
