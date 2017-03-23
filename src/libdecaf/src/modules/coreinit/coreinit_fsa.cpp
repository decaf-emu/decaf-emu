#include "coreinit_fsa.h"
#include "coreinit_messagequeue.h"

namespace coreinit
{

/**
 * Get an FSAAsyncResult from an OSMessage.
 */
FSAAsyncResult *
FSAGetAsyncResult(OSMessage *message)
{
   return be_ptr<FSAAsyncResult> { message->message };
}

namespace internal
{

/**
 * Initialise an FSAAsyncResult object.
 */
void
fsaAsyncResultInit(FSAAsyncResult *asyncResult,
                   const FSAAsyncData *asyncData,
                   OSFunctionType func)
{
   memset(asyncResult, 0, sizeof(FSAAsyncResult));
   asyncResult->userCallback = asyncData->userCallback;
   asyncResult->ioMsgQueue = asyncData->ioMsgQueue;
   asyncResult->userContext = asyncData->userContext;
   asyncResult->msg.type = func;
   asyncResult->msg.data = reinterpret_cast<void *>(asyncResult);
}

} // namespace internal

} // namespace coreinit
