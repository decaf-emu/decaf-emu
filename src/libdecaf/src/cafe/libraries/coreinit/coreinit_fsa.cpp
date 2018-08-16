#include "coreinit.h"
#include "coreinit_fsa.h"
#include "coreinit_messagequeue.h"

namespace cafe::coreinit
{

/**
 * Get an FSAAsyncResult from an OSMessage.
 */
virt_ptr<FSAAsyncResult>
FSAGetAsyncResult(virt_ptr<OSMessage> message)
{
   return virt_cast<FSAAsyncResult *>(message->message);
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
   std::memset(asyncResult.getRawPointer(), 0, sizeof(FSAAsyncResult));
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
   RegisterFunctionExport(FSAGetAsyncResult);
}

} // namespace cafe::coreinit
