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
   sFsData->initialised = false;
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


/**
 * Convert an FSAStatus error code to an FSStatus error code.
 */
FSStatus
fsDecodeFsaStatusToFsStatus(FSAStatus error)
{
   switch (error) {
   case FSAStatus::NotInit:
   case FSAStatus::Busy:
      return FSStatus::FatalError;
   case FSAStatus::Cancelled:
      return FSStatus::Cancelled;
   case FSAStatus::EndOfDir:
   case FSAStatus::EndOfFile:
      return FSStatus::End;
   case FSAStatus::MaxMountpoints:
   case FSAStatus::MaxVolumes:
   case FSAStatus::MaxClients:
   case FSAStatus::MaxFiles:
   case FSAStatus::MaxDirs:
      return FSStatus::Max;
   case FSAStatus::AlreadyOpen:
      return FSStatus::AlreadyOpen;
   case FSAStatus::AlreadyExists:
      return FSStatus::Exists;
   case FSAStatus::NotFound:
      return FSStatus::NotFound;
   case FSAStatus::NotEmpty:
      return FSStatus::Exists;
   case FSAStatus::AccessError:
      return FSStatus::AccessError;
   case FSAStatus::PermissionError:
      return FSStatus::PermissionError;
   case FSAStatus::DataCorrupted:
      return FSStatus::FatalError;
   case FSAStatus::StorageFull:
      return FSStatus::StorageFull;
   case FSAStatus::JournalFull:
      return FSStatus::JournalFull;
   case FSAStatus::LinkEntry:
      return FSStatus::FatalError;
   case FSAStatus::UnavailableCmd:
      return FSStatus::FatalError;
   case FSAStatus::UnsupportedCmd:
      return FSStatus::UnsupportedCmd;
   case FSAStatus::InvalidParam:
   case FSAStatus::InvalidPath:
   case FSAStatus::InvalidBuffer:
   case FSAStatus::InvalidAlignment:
   case FSAStatus::InvalidClientHandle:
   case FSAStatus::InvalidFileHandle:
   case FSAStatus::InvalidDirHandle:
      return FSStatus::FatalError;
   case FSAStatus::NotFile:
      return FSStatus::NotFile;
   case FSAStatus::NotDir:
      return FSStatus::NotDirectory;
   case FSAStatus::FileTooBig:
      return FSStatus::FileTooBig;
   case FSAStatus::OutOfRange:
   case FSAStatus::OutOfResources:
      return FSStatus::FatalError;
   case FSAStatus::MediaNotReady:
      return FSStatus::FatalError;
   case FSAStatus::MediaError:
      return FSStatus::MediaError;
   case FSAStatus::WriteProtected:
   case FSAStatus::InvalidMedia:
      return FSStatus::FatalError;
   }

   return FSStatus::FatalError;
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
