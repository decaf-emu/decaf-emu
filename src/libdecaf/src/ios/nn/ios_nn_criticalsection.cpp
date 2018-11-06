#include "ios_nn_criticalsection.h"
#include "ios_nn_tls.h"

#include "ios/kernel/ios_kernel_semaphore.h"
#include "ios/kernel/ios_kernel_thread.h"
#include "ios/kernel/ios_kernel_process.h"

#include <array>
#include <common/decaf_assert.h>

using namespace ios::kernel;

/**
 * nn implements N critical sections for a process using a few semaphores:
 *  - 1 semaphore per process for critical section data members.
 *  - 1 semaphore per process for condition variable data members.
 *  - 1 semaphore per thread, allocated in TLS.
 */

namespace nn
{

class CriticalSection;

struct CriticalSectionWaiter
{
   SemaphoreId semaphore;
   CriticalSection *waitObject;
   CriticalSectionWaiter *prev;
   CriticalSectionWaiter *next;
};

struct PerProcessCriticalSectionData
{
   SemaphoreId criticalSectionSemaphore { -1 };
   SemaphoreId conditionVariableSemaphore { -1 };
   TlsEntry threadSemaphoreEntry { };
   CriticalSectionWaiter *waiters { nullptr };
};

static std::array<PerProcessCriticalSectionData, NumIosProcess>
sPerProcessCriticalSectionData { };

static PerProcessCriticalSectionData &
getProcessCriticalSectionData()
{
   auto error = IOS_GetCurrentProcessId();
   decaf_check(error >= ::ios::Error::OK);
   return sPerProcessCriticalSectionData[static_cast<size_t>(error)];
}

static SemaphoreId
getCriticalSectionProcessSemaphore()
{
   return getProcessCriticalSectionData().criticalSectionSemaphore;
}
static SemaphoreId
getConditionVariableProcessSemaphore()
{
   return getProcessCriticalSectionData().conditionVariableSemaphore;
}

static SemaphoreId
getConditionVariableThreadSemaphore()
{
   auto tls = tlsGetEntry(getProcessCriticalSectionData().threadSemaphoreEntry);
   return *phys_cast<SemaphoreId *>(tls);
}

bool CriticalSection::try_lock()
{
   auto semaphore = getCriticalSectionProcessSemaphore();
   auto result = false;

   IOS_WaitSemaphore(semaphore, FALSE);
   if (!mEntered) {
      result = true;
      mEntered = true;
   }

   IOS_SignalSempahore(semaphore);
   return result;
}

void CriticalSection::lock()
{
   auto processData = getProcessCriticalSectionData();
   auto result = false;

   // Fast path, try lock with no contention
   IOS_WaitSemaphore(processData.criticalSectionSemaphore, FALSE);
   if (!mEntered) {
      mEntered = true;
      IOS_SignalSempahore(processData.criticalSectionSemaphore);
      return;
   }

   mWaiters++;
   IOS_SignalSempahore(processData.criticalSectionSemaphore);

   // Contention path, wait on condition variable
   while (true) {
      waitWaiterConditionVariable();

      IOS_WaitSemaphore(processData.criticalSectionSemaphore, FALSE);
      if (!mEntered) {
         --mWaiters;
         IOS_SignalSempahore(processData.criticalSectionSemaphore);
         return;
      }

      IOS_SignalSempahore(processData.criticalSectionSemaphore);
   }
}

void CriticalSection::unlock()
{
   auto processData = getProcessCriticalSectionData();
   auto wakeWaiters = false;

   IOS_WaitSemaphore(processData.criticalSectionSemaphore, FALSE);
   mEntered = false;
   if (mWaiters) {
      wakeWaiters = true;
   }
   IOS_SignalSempahore(processData.criticalSectionSemaphore);

   if (wakeWaiters) {
      signalWaiterConditionVariable(1);
   }
}

void CriticalSection::waitWaiterConditionVariable()
{
   auto processData = getProcessCriticalSectionData();
   IOS_WaitSemaphore(processData.conditionVariableSemaphore, FALSE);
   if (!mEntered) {
      auto waiter = CriticalSectionWaiter {
         getConditionVariableThreadSemaphore(), this,
         nullptr, nullptr
      };

      if (processData.waiters) {
         waiter.next = processData.waiters->next;
         waiter.prev = processData.waiters;

         processData.waiters->next->prev = &waiter;
         processData.waiters->next = &waiter;
      } else {
         waiter.next = &waiter;
         waiter.prev = &waiter;

         processData.waiters = &waiter;
      }

      IOS_SignalSempahore(processData.conditionVariableSemaphore);
      IOS_WaitSemaphore(waiter.semaphore, FALSE);
      decaf_check(!waiter.next && !waiter.prev);
      IOS_WaitSemaphore(processData.conditionVariableSemaphore, FALSE);
   }
   IOS_SignalSempahore(processData.conditionVariableSemaphore);
}

void CriticalSection::signalWaiterConditionVariable(int wakeCount)
{
   auto processData = getProcessCriticalSectionData();
   IOS_WaitSemaphore(processData.conditionVariableSemaphore, FALSE);

   while (wakeCount) {
      if (!processData.waiters) {
         // No waiters
         break;
      }

      // Find a waiter for this wait object
      auto waiter = static_cast<CriticalSectionWaiter *>(nullptr);
      auto start = processData.waiters;
      auto end = processData.waiters->next;

      for (auto itr = processData.waiters; itr && itr != processData.waiters->next; itr = itr->prev) {
         decaf_check(itr->next && itr->prev);

         if (itr->waitObject == this) {
            waiter = itr;
            break;
         }
      }

      if (!waiter) {
         // No waiters for this wait object
         break;
      }

      // Remove waiter from queue
      if (waiter->next == waiter) {
         // This was the only item in queue
         processData.waiters = nullptr;
      } else {
         auto prev = waiter->prev;
         auto next = waiter->next;

         prev->next = next;
         next->prev = prev;
      }

      waiter->prev = nullptr;
      waiter->next = nullptr;
      --wakeCount;
      IOS_SignalSempahore(waiter->semaphore);
   }

   IOS_SignalSempahore(processData.conditionVariableSemaphore);
}

namespace internal
{

void
initialiseProcessCriticalSectionData()
{
   auto &data = getProcessCriticalSectionData();
   data.criticalSectionSemaphore = IOS_CreateSemaphore(1, 1);
   data.conditionVariableSemaphore = IOS_CreateSemaphore(1, 1);

   tlsAllocateEntry(data.threadSemaphoreEntry,
                    [](TlsEntryEvent event,
                       phys_ptr<void> dst,
                       phys_ptr<void> copySrc) -> uint32_t
                    {
                       auto data = phys_cast<SemaphoreId *>(dst);
                       auto copyData = phys_cast<SemaphoreId *>(copySrc);

                       switch (event) {
                       case TlsEntryEvent::Create:
                          *data = IOS_CreateSemaphore(1, 0);
                          break;
                       case TlsEntryEvent::Destroy:
                          IOS_DestroySempahore(*data);
                          break;
                       case TlsEntryEvent::Copy:
                          *data = *copyData;
                          return 0u;
                       }

                       return sizeof(SemaphoreId);
                    });
}

void
freeProcessCriticalSectionData()
{
   auto &data = getProcessCriticalSectionData();
   IOS_DestroySempahore(data.criticalSectionSemaphore);
   IOS_DestroySempahore(data.conditionVariableSemaphore);
}

} // namespace internal

} // namespace nn
