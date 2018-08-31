#include "coreinit.h"
#include "coreinit_appio.h"
#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_driver.h"
#include "coreinit_fastmutex.h"

namespace cafe::coreinit
{

using ClientBodyQueue = internal::Queue<FSClientBodyQueue, FSClientBodyLink,
                                        FSClientBody, &FSClientBody::link>;

struct StaticFsData
{
   be2_val<bool> initialised;
   be2_struct<OSFastMutex> mutex;
   be2_val<uint32_t> numClients;
   be2_struct<FSClientBodyQueue> clients;
};

static virt_ptr<StaticFsData>
sFsData = nullptr;


/**
 * Initialise filesystem.
 */
void
FSInit()
{
   if (sFsData->initialised || internal::fsDriverDone()) {
      return;
   }

   sFsData->initialised = true;
   ClientBodyQueue::init(virt_addrof(sFsData->clients));
   OSFastMutex_Init(virt_addrof(sFsData->mutex), nullptr);
   internal::initialiseFsDriver();
}


/**
 * Shutdown filesystem.
 */
void
FSShutdown()
{
}


/**
 * Get an FSAsyncResult from an OSMessage.
 */
virt_ptr<FSAsyncResult>
FSGetAsyncResult(virt_ptr<OSMessage> message)
{
   return virt_cast<FSAsyncResult *>(message->message);
}


/**
 * Get the number of registered FS clients.
 */
uint32_t
FSGetClientNum()
{
   return sFsData->numClients;
}


namespace internal
{

/**
 * Returns true if filesystem has been intialised.
 */
bool
fsInitialised()
{
   return sFsData->initialised;
}


/**
 * Returns true if client is registered.
 */
bool
fsClientRegistered(virt_ptr<FSClient> client)
{
   return fsClientRegistered(fsClientGetBody(client));
}


/**
 * Returns true if client is registered.
 */
bool
fsClientRegistered(virt_ptr<FSClientBody> clientBody)
{
   OSFastMutex_Lock(virt_addrof(sFsData->mutex));
   auto registered = ClientBodyQueue::contains(virt_addrof(sFsData->clients),
                                               clientBody);
   OSFastMutex_Unlock(virt_addrof(sFsData->mutex));
   return registered;
}


/**
 * Register a client with the filesystem.
 */
bool
fsRegisterClient(virt_ptr<FSClientBody> clientBody)
{
   OSFastMutex_Lock(virt_addrof(sFsData->mutex));
   ClientBodyQueue::append(virt_addrof(sFsData->clients), clientBody);
   sFsData->numClients++;
   OSFastMutex_Unlock(virt_addrof(sFsData->mutex));
   return true;
}


/**
 * Deregister a client from the filesystem.
 */
bool
fsDeregisterClient(virt_ptr<FSClientBody> clientBody)
{
   OSFastMutex_Lock(virt_addrof(sFsData->mutex));
   ClientBodyQueue::erase(virt_addrof(sFsData->clients), clientBody);
   sFsData->numClients--;
   OSFastMutex_Unlock(virt_addrof(sFsData->mutex));
   return true;
}


/**
 * Initialise an FSAsyncResult structure for an FS command.
 *
 * \retval FSStatus::OK
 * Success.
 */
FSStatus
fsAsyncResultInit(virt_ptr<FSClientBody> clientBody,
                  virt_ptr<FSAsyncResult> asyncResult,
                  virt_ptr<const FSAsyncData> asyncData)
{
   asyncResult->asyncData = *asyncData;

   if (!asyncData->ioMsgQueue) {
      asyncResult->asyncData.ioMsgQueue = OSGetDefaultAppIOQueue();
   }

   asyncResult->client = clientBody->client;
   asyncResult->ioMsg.data = asyncResult;
   asyncResult->ioMsg.type = OSFunctionType::FsCmdAsync;
   return FSStatus::OK;
}

} // namespace internal

void
Library::registerFsSymbols()
{
   RegisterFunctionExport(FSInit);
   RegisterFunctionExport(FSShutdown);
   RegisterFunctionExport(FSGetAsyncResult);
   RegisterFunctionExport(FSGetClientNum);

   RegisterDataInternal(sFsData);
}

} // namespace cafe::coreinit
