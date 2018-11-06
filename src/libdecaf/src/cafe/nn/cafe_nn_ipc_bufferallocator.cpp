#include "cafe_nn_ipc_bufferallocator.h"

#include "cafe/libraries/coreinit/coreinit_ios.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"

using namespace cafe::coreinit;

namespace nn::ipc
{

BufferAllocator::BufferAllocator()
{
   OSInitMutex(virt_addrof(mMutex));
}

void
BufferAllocator::initialise(virt_ptr<void> buffer,
                            uint32_t size)
{
   auto numBuffers = size / BufferSize;

   // Set up a linked list of free buffers
   auto bufferAddr = virt_cast<virt_addr>(buffer);
   mFreeBuffers = virt_cast<FreeBuffer *>(bufferAddr);

   for (auto i = 0u; i < numBuffers; ++i) {
      auto curBuffer = virt_cast<FreeBuffer *>(bufferAddr);
      auto nextBuffer = virt_cast<FreeBuffer *>(bufferAddr + BufferSize);

      if (i < numBuffers - 1) {
         curBuffer->next = nextBuffer;
      } else {
         curBuffer->next = nullptr;
      }

      bufferAddr += BufferSize;
   }
}

virt_ptr<void>
BufferAllocator::allocate(uint32_t size)
{
   auto result = virt_ptr<void> { nullptr };
   decaf_check(size <= BufferSize);

   OSLockMutex(virt_addrof(mMutex));
   result = mFreeBuffers;
   mFreeBuffers = mFreeBuffers->next;
   OSUnlockMutex(virt_addrof(mMutex));
   return result;
}

void
BufferAllocator::deallocate(virt_ptr<void> ptr)
{
   auto buffer = virt_cast<FreeBuffer *>(ptr);
   OSLockMutex(virt_addrof(mMutex));
   buffer->next = mFreeBuffers;
   mFreeBuffers = buffer;
   OSUnlockMutex(virt_addrof(mMutex));
}

} // namespace nn::ipc
