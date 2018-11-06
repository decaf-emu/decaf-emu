#pragma once
#include "cafe/libraries/coreinit/coreinit_ios.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"

#include <libcpu/be2_struct.h>

namespace nn::ipc
{

class BufferAllocator
{
   static constexpr auto BufferSize = 256u;

   struct FreeBuffer
   {
      be2_virt_ptr<FreeBuffer> next;
   };

public:
   BufferAllocator();

   void
   initialise(virt_ptr<void> buffer,
              uint32_t size);

   virt_ptr<void>
   allocate(uint32_t size);

   void
   deallocate(virt_ptr<void> ptr);

private:
   be2_struct<cafe::coreinit::OSMutex> mMutex;
   be2_virt_ptr<FreeBuffer> mFreeBuffers;
};

} // namespace nn::ipc
