#pragma once
#include "ios_nn_criticalsection.h"

#include "ios/kernel/ios_kernel_thread.h"

namespace nn
{

class RecursiveMutex
{
public:
   void lock();
   bool try_lock();
   void unlock();

   bool locked();

private:
   int mRecursionCount = 0;
   ios::kernel::ThreadId mOwnerThread = -1;
   CriticalSection mCriticalSection;
};

} // namespace nn
