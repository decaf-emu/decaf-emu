#pragma once
#include "ios/kernel/ios_kernel_thread.h"
#include "nn/nn_result.h"

namespace nn
{

class Thread
{
public:
   using id = ::ios::kernel::ThreadId;
   using native_handle_type = ::ios::kernel::ThreadId;

   Thread() noexcept = default;
   Thread(const Thread &) = delete;
   Thread(Thread &&other) noexcept;
   ~Thread();

   Result start(::ios::kernel::ThreadEntryFn entry,
                phys_ptr<void> context,
                phys_ptr<uint8_t> stackTop,
                uint32_t stackSize,
                ::ios::kernel::ThreadPriority priority);
   void join();
   void detach();
   void swap(Thread &other) noexcept;

   id get_id() const noexcept;
   native_handle_type native_handle() const noexcept;
   bool joinable() const noexcept;

   static unsigned int hardware_concurrency() noexcept
   {
      return 1;
   }

private:
   native_handle_type mThreadId = -1;
   bool mJoined = true;
};

} // namespace nn
