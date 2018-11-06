#pragma once

namespace nn
{

class CriticalSection
{
public:
   void lock();
   bool try_lock();
   void unlock();

private:
   void waitWaiterConditionVariable();
   void signalWaiterConditionVariable(int wakeCount);

private:
   bool mEntered = false;
   int mWaiters = 0;
};

namespace internal
{

void
initialiseProcessCriticalSectionData();

void
freeProcessCriticalSectionData();

} // namespace internal

} // namespace nn
