#include "gpu_ih.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace gpu::ih
{

static std::vector<Entry>
mWriteVector;

static std::vector<Entry>
mReadVector;

static std::mutex
sMutex;

static InterruptCallbackFn
sInterruptCallback = nullptr;

static std::atomic<uint32_t>
sInterruptControl { 0u };

void
write(const Entries &entries)
{
   auto generateInterrupt = false;

   {
      std::unique_lock<std::mutex> lock { sMutex };
      generateInterrupt = mWriteVector.empty();
      mWriteVector.insert(mWriteVector.end(), entries.begin(), entries.end());
   }

   if (generateInterrupt && sInterruptCallback) {
      sInterruptCallback();
   }
}

Entries
read()
{
   std::unique_lock<std::mutex> lock { sMutex };
   mReadVector.clear();
   mReadVector.swap(mWriteVector);
   return mReadVector;
}

/**
 * Set callback to be called when a GPU interrupt is triggered.
 */
void
setInterruptCallback(InterruptCallbackFn callback)
{
   sInterruptCallback = callback;
}

void
enable(latte::CP_INT_CNTL cntl)
{
   sInterruptControl |= cntl.value;
}

void
disable(latte::CP_INT_CNTL cntl)
{
   sInterruptControl &= ~cntl.value;
}

} // namespace gpu::ih
