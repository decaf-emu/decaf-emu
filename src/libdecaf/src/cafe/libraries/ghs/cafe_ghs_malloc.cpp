#include "cafe_ghs_malloc.h"

#include "cafe/libraries/coreinit/coreinit_memdefaultheap.h"
#include "cafe/libraries/coreinit/coreinit_ghs.h"
#include "cafe/libraries/coreinit/coreinit_osreport.h"

namespace cafe::ghs
{

using namespace cafe::coreinit;

struct MallocGuard
{
   static constexpr uint32_t GuardWord = 0xCAFE4321;
   be2_val<uint32_t> guardWord;
   be2_val<uint32_t> allocSize;
};
CHECK_OFFSET(MallocGuard, 0, guardWord);
CHECK_OFFSET(MallocGuard, 4, allocSize);
CHECK_SIZE(MallocGuard, 8);

virt_ptr<void>
malloc(uint32_t size)
{
   auto ptr = MEMAllocFromDefaultHeapEx(size + sizeof(MallocGuard), 8);
   if (!ptr) {
      gh_set_errno(12);
      return nullptr;
   }

   auto guard = virt_cast<MallocGuard *>(ptr);
   guard->guardWord = MallocGuard::GuardWord;
   guard->allocSize = size;
   return virt_cast<void *>(guard + 1);
}

void
free(virt_ptr<void> ptr)
{
   if (!ptr) {
      return;
   }


   auto guard = virt_cast<MallocGuard *>(ptr) - 1;
   if (guard->guardWord != MallocGuard::GuardWord) {
      internal::OSPanic("cos_def_malloc.c", 93,
                        "Failed assertion *rawptr == COS_DEF_MALLOC_GUARDWORD");
   }

   MEMFreeToDefaultHeap(ptr);
}

} // namespace cafe::ghs
