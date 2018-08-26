#include "coreinit.h"
#include "coreinit_systemmessagequeue.h"

namespace cafe::coreinit
{

constexpr auto SystemMessageQueueLength = 16u;

struct StaticSystemMessageQueueData
{
   be2_struct<OSMessageQueue> queue;
   be2_array<char, 32> queueName;
   be2_array<OSMessage, SystemMessageQueueLength> messages;
};

static virt_ptr<StaticSystemMessageQueueData>
sSystemMessageQueueData = nullptr;


/**
 * Get a pointer to the system message queue.
 *
 * Used for acquiring & releasing foreground messages.
 */
virt_ptr<OSMessageQueue>
OSGetSystemMessageQueue()
{
   return virt_addrof(sSystemMessageQueueData->queue);
}

namespace internal
{

void
initialiseSystemMessageQueue()
{
   sSystemMessageQueueData->queueName = "{ SystemMQ }";
   OSInitMessageQueueEx(virt_addrof(sSystemMessageQueueData->queue),
                        virt_addrof(sSystemMessageQueueData->messages),
                        sSystemMessageQueueData->messages.size(),
                        virt_addrof(sSystemMessageQueueData->queueName));
}

} // namespace internal

void
Library::registerSystemMessageQueueSymbols()
{
   RegisterFunctionExport(OSGetSystemMessageQueue);
   RegisterDataInternal(sSystemMessageQueueData);
}

} // namespace cafe::coreinit
