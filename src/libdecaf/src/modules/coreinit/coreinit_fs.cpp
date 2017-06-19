#include "coreinit.h"
#include "coreinit_appio.h"
#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_driver.h"
#include "coreinit_fastmutex.h"

namespace coreinit
{

using ClientBodyQueue = internal::Queue<FSClientBodyQueue, FSClientBodyLink,
                                        FSClientBody, &FSClientBody::link>;

struct FsData
{
   bool initialised;
   OSFastMutex mutex;
   uint32_t numClients;
   FSClientBodyQueue clients;
};

static FsData *
sFsData;


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
   ClientBodyQueue::init(&sFsData->clients);
   OSFastMutex_Init(&sFsData->mutex, nullptr);
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
FSAsyncResult *
FSGetAsyncResult(OSMessage *message)
{
   return be_ptr<FSAsyncResult> { message->message };
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
fsClientRegistered(FSClient *client)
{
   return fsClientRegistered(fsClientGetBody(client));
}


/**
 * Returns true if client is registered.
 */
bool
fsClientRegistered(FSClientBody *clientBody)
{
   OSFastMutex_Lock(&sFsData->mutex);
   bool registered = ClientBodyQueue::contains(&sFsData->clients, clientBody);
   OSFastMutex_Unlock(&sFsData->mutex);
   return registered;
}


/**
 * Register a client with the filesystem.
 */
bool
fsRegisterClient(FSClientBody *clientBody)
{
   OSFastMutex_Lock(&sFsData->mutex);
   ClientBodyQueue::append(&sFsData->clients, clientBody);
   sFsData->numClients++;
   OSFastMutex_Unlock(&sFsData->mutex);
   return true;
}


/**
 * Deregister a client from the filesystem.
 */
bool
fsDeregisterClient(FSClientBody *clientBody)
{
   OSFastMutex_Lock(&sFsData->mutex);
   ClientBodyQueue::erase(&sFsData->clients, clientBody);
   sFsData->numClients--;
   OSFastMutex_Unlock(&sFsData->mutex);
   return true;
}


/**
 * Initialise an FSAsyncResult structure for an FS command.
 *
 * \retval FSStatus::OK
 * Success.
 */
FSStatus
fsAsyncResultInit(FSClientBody *clientBody,
                  FSAsyncResult *asyncResult,
                  const FSAsyncData *asyncData)
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
Module::registerFsFunctions()
{
   RegisterKernelFunction(FSInit);
   RegisterKernelFunction(FSShutdown);
   RegisterKernelFunction(FSGetAsyncResult);
   RegisterKernelFunction(FSGetClientNum);

   RegisterInternalData(sFsData);
}

} // namespace coreinit
