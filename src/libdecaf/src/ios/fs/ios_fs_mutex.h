#pragma once
#include "ios/kernel/ios_kernel_semaphore.h"

#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::fs::internal
{

struct Mutex
{
   be2_val<kernel::SemaphoreId> semaphore;
};
CHECK_OFFSET(Mutex, 0x0, semaphore);
CHECK_SIZE(Mutex, 0x4);

Error
initMutex(phys_ptr<Mutex> mutex);

Error
destroyMutex(phys_ptr<Mutex> mutex);

Error
lockMutex(phys_ptr<Mutex> mutex);

Error
unlockMutex(phys_ptr<Mutex> mutex);

} // namespace ios::fs::internal
