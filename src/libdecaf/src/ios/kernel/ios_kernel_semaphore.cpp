#include "ios_kernel_process.h"
#include "ios_kernel_semaphore.h"
#include "ios_kernel_scheduler.h"
#include "ios_kernel_thread.h"
#include <array>
#include <mutex>

namespace ios::kernel
{

struct StaticSemaphoreData
{
   be2_val<uint32_t> numCreatedSemaphores = uint32_t { 0 };
   be2_val<int16_t> firstFreeSemaphoreIndex = int16_t { 0 };
   be2_val<int16_t> lastFreeSemaphoreIndex = int16_t { 0 };
   be2_array<Semaphore, 750> semaphores;
};

static phys_ptr<StaticSemaphoreData>
sData;

static phys_ptr<Semaphore>
getSemaphore(SemaphoreId id)
{
   auto idx = static_cast<size_t>(id & 0xFFF);
   if (idx >= sData->semaphores.size()) {
      return nullptr;
   }

   auto semaphore = phys_addrof(sData->semaphores[idx]);
   if (semaphore->pid != internal::getCurrentProcessId()) {
      // Can only access semaphores belonging to same process.
      return nullptr;
   }

   return semaphore;
}

Error
IOS_CreateSemaphore(int32_t maxCount,
                    int32_t initialCount)
{
   if (sData->firstFreeSemaphoreIndex < 0) {
      return Error::Max;
   }

   auto semaphore = phys_addrof(sData->semaphores[sData->firstFreeSemaphoreIndex]);
   auto semaphoreId = sData->firstFreeSemaphoreIndex;

   // Remove semaphore from free semaphore linked list
   sData->firstFreeSemaphoreIndex = semaphore->nextFreeSemaphoreIndex;

   if (semaphore->nextFreeSemaphoreIndex >= 0) {
      sData->semaphores[semaphore->nextFreeSemaphoreIndex].prevFreeSemaphoreIndex = int16_t { -1 };
   } else {
      sData->lastFreeSemaphoreIndex = int16_t { -1 };
   }

   semaphore->nextFreeSemaphoreIndex = int16_t { -1 };
   semaphore->prevFreeSemaphoreIndex = int16_t { -1 };

   sData->numCreatedSemaphores++;
   semaphore->id = static_cast<SemaphoreId>(semaphoreId | (sData->numCreatedSemaphores << 12));
   semaphore->count = initialCount;
   semaphore->maxCount = maxCount;
   semaphore->pid = internal::getCurrentProcessId();
   semaphore->unknown0x04 = nullptr;
   ThreadQueue_Initialise(phys_addrof(semaphore->waitThreadQueue));

   return static_cast<Error>(semaphore->id);
}

Error
IOS_DestroySempahore(SemaphoreId id)
{
   auto semaphore = getSemaphore(id);
   if (!semaphore) {
      return Error::Invalid;
   }

   semaphore->count = 0;
   semaphore->maxCount = 0;

   // Add semaphore to the free semaphore linked list.
   auto index = static_cast<int16_t>(semaphore - phys_addrof(sData->semaphores));
   auto prevSemaphoreIndex = sData->lastFreeSemaphoreIndex;

   if (prevSemaphoreIndex >= 0) {
      auto prevSemaphore = phys_addrof(sData->semaphores[sData->lastFreeSemaphoreIndex]);
      prevSemaphore->nextFreeSemaphoreIndex = index;
   }

   semaphore->prevFreeSemaphoreIndex = prevSemaphoreIndex;
   semaphore->nextFreeSemaphoreIndex = int16_t { -1 };

   sData->lastFreeSemaphoreIndex = index;

   if (sData->firstFreeSemaphoreIndex < 0) {
      sData->firstFreeSemaphoreIndex = index;
   }

   internal::wakeupAllThreads(phys_addrof(semaphore->waitThreadQueue),
                              Error::Intr);
   internal::reschedule();
   return Error::Invalid;
}

Error
IOS_WaitSemaphore(SemaphoreId id,
                  BOOL tryWait)
{
   auto semaphore = getSemaphore(id);
   if (!semaphore) {
      return Error::Invalid;
   }

   if (semaphore->count <= 0 && tryWait) {
      return Error::SemUnavailable;
   }

   while (semaphore->count <= 0) {
      internal::sleepThread(phys_addrof(semaphore->waitThreadQueue));
      internal::reschedule();

      auto thread = internal::getCurrentThread();
      if (thread->context.queueWaitResult != Error::OK) {
         return thread->context.queueWaitResult;
      }
   }

   semaphore->count -= 1;
   return Error::OK;
}

Error
IOS_SignalSempahore(SemaphoreId id)
{
   auto semaphore = getSemaphore(id);
   if (!semaphore) {
      return Error::Invalid;
   }

   if (semaphore->count < semaphore->maxCount) {
      semaphore->count += 1;
   }

   internal::wakeupOneThread(phys_addrof(semaphore->waitThreadQueue), Error::OK);
   internal::reschedule();
   return Error::OK;
}

namespace internal
{

void
initialiseStaticSemaphoreData()
{
   sData = allocProcessStatic<StaticSemaphoreData>();
}

} // namespace internal

} // namespace ios::kernel
