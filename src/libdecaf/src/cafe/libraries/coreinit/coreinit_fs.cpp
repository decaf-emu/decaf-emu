#include "coreinit.h"
#include "coreinit_appio.h"
#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_driver.h"
#include "coreinit_fastmutex.h"

namespace cafe::coreinit
{

struct StaticFsData
{
   be2_val<bool> initialised;
   be2_array<char, 32> clientListMutexName;
   be2_struct<OSFastMutex> clientListMutex;
   be2_val<uint32_t> numClients;
   be2_virt_ptr<FSClientBody> clientList;
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
   sFsData->clientList = nullptr;
   sFsData->numClients = 0u;
   sFsData->clientListMutexName = "{ FSClient }";
   OSFastMutex_Init(virt_addrof(sFsData->clientListMutex),
                    virt_addrof(sFsData->clientListMutexName));
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
   auto registered = false;
   OSFastMutex_Lock(virt_addrof(sFsData->clientListMutex));

   if (sFsData->clientList) {
      auto itr = sFsData->clientList;
      do {
         if (itr == clientBody) {
            registered = true;
            break;
         }

         itr = itr->link.next;
      } while (itr != sFsData->clientList);
   }

   OSFastMutex_Unlock(virt_addrof(sFsData->clientListMutex));
   return registered;
}


/**
 * Register a client with the filesystem.
 */
bool
fsRegisterClient(virt_ptr<FSClientBody> clientBody)
{
   OSFastMutex_Lock(virt_addrof(sFsData->clientListMutex));

   if (sFsData->clientList) {
      auto first = sFsData->clientList;
      auto next = first->link.next;
      first->link.next = clientBody;
      next->link.prev = clientBody;
      clientBody->link.next = next;
      clientBody->link.prev = first;
   } else {
      sFsData->clientList = clientBody;
      clientBody->link.next = clientBody;
      clientBody->link.prev = clientBody;
   }

   sFsData->numClients++;
   OSFastMutex_Unlock(virt_addrof(sFsData->clientListMutex));
   return true;
}


/**
 * Deregister a client from the filesystem.
 */
bool
fsDeregisterClient(virt_ptr<FSClientBody> clientBody)
{
   OSFastMutex_Lock(virt_addrof(sFsData->clientListMutex));

   // If was first item in the list, update list pointer
   if (sFsData->clientList == clientBody) {
      if (clientBody->link.prev != clientBody) {
         sFsData->clientList = clientBody->link.prev;
      } else {
         sFsData->clientList = nullptr;
      }
   }

   // Remove from list
   auto next = clientBody->link.next;
   auto prev = clientBody->link.prev;

   prev->link.next = next;
   next->link.prev = prev;

   clientBody->link.next = nullptr;
   clientBody->link.prev = nullptr;

   sFsData->numClients--;
   OSFastMutex_Unlock(virt_addrof(sFsData->clientListMutex));
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
