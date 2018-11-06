#include "ios_nn_thread.h"
#include "ios_nn_tls.h"

#include "ios/kernel/ios_kernel_thread.h"
#include "nn/ios/nn_ios_error.h"

#include <common/decaf_assert.h>

using namespace ios::kernel;

namespace nn
{

Thread::Thread(Thread &&other) noexcept
{
   mThreadId = other.mThreadId;
   mJoined = other.mJoined;

   other.mThreadId = -1;
   other.mJoined = true;
}

Thread::~Thread()
{
   decaf_check(!joinable());
}

struct ThreadStartData
{
   ThreadEntryFn entry;
   phys_ptr<void> context;
   phys_ptr<void> tlsData;
};

static ::ios::Error
threadEntryPoint(phys_ptr<void> arg)
{
   auto startData = phys_cast<ThreadStartData *>(arg);
   tlsInitialiseThread(startData->tlsData);
   auto result = startData->entry(startData->context);
   tlsDestroyData(startData->tlsData);
   return result;
}

Result
Thread::start(ThreadEntryFn entry,
              phys_ptr<void> context,
              phys_ptr<uint8_t> stackTop,
              uint32_t stackSize,
              ThreadPriority priority)
{
   auto tlsDataSize = tlsGetDataSize();
   auto userStackTop = stackTop;

   // Allocate TLS data from stack
   stackTop = align_down(stackTop - tlsDataSize, 8);
   auto tlsData = phys_cast<void *>(stackTop);
   tlsInitialiseData(tlsData);

   // Allocate thread start context from stack
   stackTop = align_down(stackTop - sizeof(ThreadStartData), 8);
   auto threadStartData = phys_cast<ThreadStartData *>(stackTop);
   threadStartData->entry = entry;
   threadStartData->context = context;
   threadStartData->tlsData = tlsData;

   // Calculate new stack size
   stackSize -= static_cast<uint32_t>(userStackTop - stackTop);

   auto error = IOS_CreateThread(threadEntryPoint,
                                 threadStartData,
                                 stackTop,
                                 stackSize,
                                 priority,
                                 ThreadFlags::AllocateTLS);
   if (error < ::ios::Error::OK) {
      return ios::convertError(error);
   }

   mThreadId = static_cast<ThreadId>(error);

   error = IOS_StartThread(mThreadId);
   if (error < ::ios::Error::OK) {
      return ios::convertError(error);
   }

   mJoined = false;
   return ios::ResultOK;
}

void
Thread::join()
{
   if (joinable()) {
      IOS_JoinThread(mThreadId, nullptr);
      mJoined = true;
   }
}

void
Thread::detach()
{
   mThreadId = -1;
   mJoined = true;
}

void
Thread::swap(Thread &other) noexcept
{
   std::swap(mThreadId, other.mThreadId);
   std::swap(mJoined, other.mJoined);
}

Thread::id
Thread::get_id() const noexcept
{
   return mThreadId;
}

Thread::native_handle_type
Thread::native_handle() const noexcept
{
   return mThreadId;
}

bool
Thread::joinable() const noexcept
{
   return mThreadId != -1;
}

} // namespace nn
