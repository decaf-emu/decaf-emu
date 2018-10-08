#include "gpu_ringbuffer.h"

#include <condition_variable>
#include <mutex>
#include <vector>

namespace gpu::ringbuffer
{

static std::vector<uint32_t>
mWriteVector;

static std::vector<uint32_t>
mReadVector;

static std::mutex
sMutex;

static std::condition_variable
sConditionVariable;

void
write(const Buffer &items)
{
   std::unique_lock<std::mutex> lock { sMutex };
   mWriteVector.insert(mWriteVector.end(), items.begin(), items.end());
   sConditionVariable.notify_all();
}

Buffer
read()
{
   std::unique_lock<std::mutex> lock { sMutex };
   mReadVector.clear();
   mReadVector.swap(mWriteVector);
   return mReadVector;
}

bool
wait()
{
   std::unique_lock<std::mutex> lock { sMutex };
   if (mWriteVector.empty()) {
      sConditionVariable.wait(lock);
   }

   return !mWriteVector.empty();
}

void
wake()
{
   sConditionVariable.notify_all();
}

} // namespace gpu::ringbuffer
