#include "ios_fs_mutex.h"

namespace ios::fs
{

Error
initMutex(phys_ptr<Mutex> mutex)
{
   auto error = kernel::IOS_CreateSemaphore(1, 1);
   mutex->semaphore = static_cast<kernel::SemaphoreId>(error);
   return error;
}

Error
destroyMutex(phys_ptr<Mutex> mutex)
{
   auto semaphore = mutex->semaphore;
   if (mutex->semaphore < 0) {
      return Error::OK;
   }

   mutex->semaphore = static_cast<kernel::SemaphoreId>(Error::Invalid);
   return kernel::IOS_DestroySempahore(mutex->semaphore);
}

Error
lockMutex(phys_ptr<Mutex> mutex)
{
   return kernel::IOS_WaitSemaphore(mutex->semaphore, FALSE);
}

Error
unlockMutex(phys_ptr<Mutex> mutex)
{
   return kernel::IOS_SignalSempahore(mutex->semaphore);
}

} // namespace ios::fs
