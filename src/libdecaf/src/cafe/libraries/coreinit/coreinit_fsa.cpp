#include "coreinit.h"
#include "coreinit_appio.h"
#include "coreinit_fsa.h"
#include "coreinit_fsa_shim.h"
#include "coreinit_ipcbufpool.h"
#include "coreinit_messagequeue.h"
#include "coreinit_spinlock.h"

namespace cafe::coreinit
{

struct StaticFsaData
{
   be2_val<BOOL> initialised;

   be2_struct<OSSpinLock> bufPoolLock;
   be2_array<uint8_t, 0x37500> ipcPoolBuffer;
   be2_val<uint32_t> ipcPoolNumItems;
   be2_virt_ptr<IPCBufPool> ipcPool;

   be2_struct<OSSpinLock> clientListLock;
   be2_val<uint32_t> activeClientCount;
   be2_array<FSAClient, 0x40> clientList;
};

static virt_ptr<StaticFsaData>
sFsaData = nullptr;


/**
 * Initialise FSA.
 */
FSAStatus
FSAInit()
{
   if (sFsaData->initialised) {
      return FSAStatus::OK;
   }

   sFsaData->initialised = TRUE;

   // Initialise buffer pool
   OSInitSpinLock(virt_addrof(sFsaData->bufPoolLock));
   OSAcquireSpinLock(virt_addrof(sFsaData->bufPoolLock));
   if (!sFsaData->ipcPool) {
      sFsaData->ipcPool =
         IPCBufPoolCreate(virt_addrof(sFsaData->ipcPoolBuffer),
                          sFsaData->ipcPoolBuffer.size(),
                          sizeof(FSAShimBuffer),
                          virt_addrof(sFsaData->ipcPoolNumItems),
                          0);
   }
   OSReleaseSpinLock(virt_addrof(sFsaData->bufPoolLock));

   // Initialise client list
   OSInitSpinLock(virt_addrof(sFsaData->clientListLock));
   OSAcquireSpinLock(virt_addrof(sFsaData->clientListLock));
   std::memset(virt_addrof(sFsaData->clientList).get(),
               0,
               sizeof(FSAClient) * sFsaData->clientList.size());

   auto i = 0u;
   for (auto &client : sFsaData->clientList) {
      client.fsaHandle = -1;
      client.attachMessage.data = virt_addrof(client);
      client.attachMessage.type = OSFunctionType::FsaAttachEvent;

      FSAShimAllocateBuffer(virt_addrof(client.attachShimBuffer));
      client.attachShimBuffer->command = FSACommand::GetAttach;
      ++i;
   }

   OSReleaseSpinLock(virt_addrof(sFsaData->clientListLock));
   return FSAStatus::OK;
}


/**
 * Shutdown FSA.
 */
FSAStatus
FSAShutdown()
{
   return FSAStatus::OK;
}


/**
 * Create a FSA client.
 */
FSAStatus
FSAAddClient(virt_ptr<FSAClientAttachAsyncData> attachAsyncData)
{
   if (attachAsyncData) {
      attachAsyncData->ioMsgQueue = OSGetDefaultAppIOQueue();
   }

   if (sFsaData->activeClientCount == sFsaData->clientList.size()) {
      return FSAStatus::MaxClients;
   }

   if (attachAsyncData) {
      if (!attachAsyncData->ioMsgQueue) {
         return FSAStatus::InvalidParam;
      }

      if (attachAsyncData->ioMsgQueue == OSGetDefaultAppIOQueue() &&
          !attachAsyncData->userCallback) {
         return FSAStatus::InvalidParam;
      }
   }

   auto error = internal::fsaShimOpen();
   if (error < IOSError::OK) {
      return FSAStatus::PermissionError;
   }

   auto fsaHandle = static_cast<FSAClientHandle>(error);

   OSAcquireSpinLock(virt_addrof(sFsaData->clientListLock));
   auto client = virt_ptr<FSAClient> { nullptr };

   for (auto &clientItr : sFsaData->clientList) {
      if (clientItr.state != FSAClientState::Free) {
         continue;
      }

      client = virt_addrof(clientItr);
   }

   if (!client) {
      OSReleaseSpinLock(virt_addrof(sFsaData->clientListLock));
      IOS_Close(fsaHandle);
      return FSAStatus::MaxClients;
   }

   client->state = FSAClientState::Allocated;
   client->fsaHandle = fsaHandle;
   OSInitEvent(virt_addrof(client->attachEvent), FALSE, OSEventMode::AutoReset);

   if (attachAsyncData) {
      decaf_abort("Unimplemented FSAClient with attachAsyncData");
   } else {
      client->asyncAttachData.userCallback = nullptr;
      client->asyncAttachData.userContext = nullptr;
      client->asyncAttachData.ioMsgQueue = nullptr;
   }

   ++sFsaData->activeClientCount;
   OSReleaseSpinLock(virt_addrof(sFsaData->clientListLock));
   return static_cast<FSAStatus>(fsaHandle);
}


/**
 * Delete a FSA client.
 */
FSAStatus
FSADelClient(FSAClientHandle handle)
{
   auto client = FSAShimCheckClientHandle(handle);
   if (!client) {
      return FSAStatus::InvalidClientHandle;
   }

   // Start closing the client
   client->state = FSAClientState::Closing;

   auto error = internal::fsaShimClose(client->fsaHandle);
   if (error < IOSError::OK) {
      // Failed to close the client...
      client->state = FSAClientState::Allocated;
      return FSAShimDecodeIosErrorToFsaStatus(handle, error);
   }

   if (client->asyncAttachData.userCallback || client->asyncAttachData.ioMsgQueue) {
      // Wait for the GetAttach to callback due to handle closing
      OSWaitEvent(virt_addrof(client->attachEvent));
   }

   OSAcquireSpinLock(virt_addrof(sFsaData->clientListLock));
   client->state = FSAClientState::Free;
   client->fsaHandle = -1;
   client->asyncAttachData.userCallback = nullptr;
   client->asyncAttachData.userContext = nullptr;
   client->asyncAttachData.ioMsgQueue = nullptr;
   --sFsaData->activeClientCount;
   OSReleaseSpinLock(virt_addrof(sFsaData->clientListLock));

   return FSAStatus::OK;
}


/**
 * Get an FSAAsyncResult from an OSMessage.
 */
virt_ptr<FSAAsyncResult>
FSAGetAsyncResult(virt_ptr<OSMessage> message)
{
   return virt_cast<FSAAsyncResult *>(message->message);
}


/**
 * __FSAShimCheckClientHandle
 *
 * Check if a FSAClientHandle is valid.
 */
virt_ptr<FSAClient>
FSAShimCheckClientHandle(FSAClientHandle handle)
{
   if (handle < 0) {
      return nullptr;
   }

   OSAcquireSpinLock(virt_addrof(sFsaData->clientListLock));

   for (auto &client : sFsaData->clientList) {
      if (client.state == FSAClientState::Allocated &&
          client.fsaHandle == handle) {
         OSReleaseSpinLock(virt_addrof(sFsaData->clientListLock));
         return virt_addrof(client);
      }
   }

   OSReleaseSpinLock(virt_addrof(sFsaData->clientListLock));
   return nullptr;
}


/**
 * __FSAShimAllocateBuffer
 *
 * Allocate a FSAShimBuffer object from the FSA ipc buffer pool.
 */
FSAStatus
FSAShimAllocateBuffer(virt_ptr<virt_ptr<FSAShimBuffer>> outBuffer)
{
   if (!sFsaData->ipcPool) {
      return FSAStatus::NotInit;
   }

   OSAcquireSpinLock(virt_addrof(sFsaData->bufPoolLock));
   auto ptr = IPCBufPoolAllocate(sFsaData->ipcPool, sizeof(FSAShimBuffer));
   OSReleaseSpinLock(virt_addrof(sFsaData->bufPoolLock));

   if (!ptr) {
      return FSAStatus::OutOfResources;
   }

   std::memset(ptr.get(), 0, sizeof(FSAShimBuffer));
   *outBuffer = virt_cast<FSAShimBuffer *>(ptr);
   return FSAStatus::OK;
}


/**
 * __FSAShimFreeBuffer
 *
 * Allocate a FSAShimBuffer object from the FSA ipc buffer pool.
 */
void
FSAShimFreeBuffer(virt_ptr<FSAShimBuffer> buffer)
{
   OSAcquireSpinLock(virt_addrof(sFsaData->bufPoolLock));
   IPCBufPoolFree(sFsaData->ipcPool, buffer);
   OSReleaseSpinLock(virt_addrof(sFsaData->bufPoolLock));
}


namespace internal
{


/**
 * Initialise an FSAAsyncResult object.
 */
void
fsaAsyncResultInit(virt_ptr<FSAAsyncResult> asyncResult,
                   virt_ptr<const FSAAsyncData> asyncData,
                   OSFunctionType func)
{
   std::memset(asyncResult.get(), 0, sizeof(FSAAsyncResult));
   asyncResult->userCallback = asyncData->userCallback;
   asyncResult->ioMsgQueue = asyncData->ioMsgQueue;
   asyncResult->userContext = asyncData->userContext;
   asyncResult->msg.type = func;
   asyncResult->msg.data = asyncResult;
}


/**
 * Convert an FSAStatus error code to an FSStatus error code.
 */
FSStatus
fsaDecodeFsaStatusToFsStatus(FSAStatus error)
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
Library::registerFsaSymbols()
{
   RegisterFunctionExport(FSAInit);
   RegisterFunctionExport(FSAShutdown);
   RegisterFunctionExport(FSAAddClient);
   RegisterFunctionExport(FSADelClient);
   RegisterFunctionExport(FSAGetAsyncResult);
   RegisterFunctionExportName("__FSAShimAllocateBuffer",
                              FSAShimAllocateBuffer);
   RegisterFunctionExportName("__FSAShimFreeBuffer",
                              FSAShimFreeBuffer);

   RegisterDataInternal(sFsaData);
}

} // namespace cafe::coreinit
