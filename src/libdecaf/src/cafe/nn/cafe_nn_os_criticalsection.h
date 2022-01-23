#pragma once
#include "cafe/libraries/coreinit/coreinit_fastmutex.h"

#include <libcpu/be2_struct.h>

namespace nn::os
{

struct CriticalSection
{
   CriticalSection()
   {
      cafe::coreinit::OSFastMutex_Init(virt_addrof(_mutex), nullptr);
   }

   void lock()
   {
      cafe::coreinit::OSFastMutex_Lock(virt_addrof(_mutex));
   }

   bool try_lock()
   {
      return !!cafe::coreinit::OSFastMutex_TryLock(virt_addrof(_mutex));
   }

   void unlock()
   {
      cafe::coreinit::OSFastMutex_Unlock(virt_addrof(_mutex));
   }

   be2_struct<cafe::coreinit::OSFastMutex> _mutex;
};
CHECK_OFFSET(CriticalSection, 0x00, _mutex);
CHECK_SIZE(CriticalSection, 0x2C);

} // namespace nn::os
