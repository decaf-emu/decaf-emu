#include "coreinit.h"
#include "coreinit_ghs.h"
#include "coreinit_interrupts.h"
#include "coreinit_spinlock.h"
#include "coreinit_memheap.h"
#include "coreinit_mutex.h"
#include "coreinit_scheduler.h"
#include "libcpu/trace.h"
#include "ppcutils/wfunc_call.h"
#include "common/decaf_assert.h"

namespace coreinit
{

static OSSpinLock *
ghsSpinLock;

be_val<uint32_t>*
p__gh_errno;

be_wfunc_ptr<void>*
p__atexit_cleanup;

be_wfunc_ptr<void>*
p__stdio_cleanup;

be_wfunc_ptr<void, be_ptr<void>*>*
p__cpp_exception_init_ptr;

be_wfunc_ptr<void, be_ptr<void>*>*
p__cpp_exception_cleanup_ptr;

be_val<uint16_t>*
p__gh_FOPEN_MAX;

__ghs_iob* p_iob;

__ghs_iob_lock* p_iob_lock;

BOOL
ghsLock()
{
   return OSAcquireSpinLock(ghsSpinLock);
}

BOOL
ghsUnlock()
{
   return OSReleaseSpinLock(ghsSpinLock);
}

be_ptr<void>*
ghs_flock_ptr(void *file)
{
   unsigned int index = (unsigned int)(((_ghs_iobuf*)file) - (_ghs_iobuf*)p_iob);

   if (index > *p__gh_FOPEN_MAX) {
      index = *p__gh_FOPEN_MAX;
   }

   return &(*p_iob_lock)[index];
}

be_val<uint32_t>*
ghs_errno_ptr()
{
   return p__gh_errno;
}

uint32_t
ghs_get_errno()
{
   return *p__gh_errno;
}

void
ghs_set_errno(uint32_t err)
{
   *p__gh_errno = err;
}

void *
ghs_get_eh_globals()
{
   return OSGetCurrentThread()->_ghs__eh_globals;
}

void *
ghs_get_eh_init_block()
{
   return nullptr;
}

void *
ghs_get_eh_mem_manage()
{
   return &OSGetCurrentThread()->_ghs__eh_mem_manage;
}

void *
ghs_get_eh_store_globals()
{
   return &OSGetCurrentThread()->_ghs__eh_store_globals;
}

void *
ghs_get_eh_store_globals_tdeh()
{
   return &OSGetCurrentThread()->_ghs__eh_store_globals_tdeh;
}

void
ghs_flock_file(uint32_t lockIx)
{
   // TODO: ghs_flock_file
   decaf_check(lockIx <= GHS_FOPEN_MAX);
   // Lock mutex for file lockIx
}

void ghs_mtx_init(void *mtx)
{
   be_ptr<OSMutex> *pmutex = static_cast<be_ptr<OSMutex>*>(mtx);
   *pmutex = static_cast<OSMutex*>((*pMEMAllocFromDefaultHeapEx)(sizeof(OSMutex), 8));
   OSInitMutex(*pmutex);
}

void ghs_mtx_dst(void *mtx)
{
   be_ptr<OSMutex> *pmutex = static_cast<be_ptr<OSMutex>*>(mtx);
   (*pMEMFreeToDefaultHeap)(*pmutex);
   *pmutex = nullptr;
}

void ghs_mtx_lock(void *mtx)
{
   be_ptr<OSMutex> *pmutex = static_cast<be_ptr<OSMutex>*>(mtx);
   OSLockMutex(*pmutex);
}

void ghs_mtx_unlock(void *mtx)
{
   be_ptr<OSMutex> *pmutex = static_cast<be_ptr<OSMutex>*>(mtx);
   OSUnlockMutex(*pmutex);
}

namespace internal
{

void ghsSetupExceptions()
{
   if (p__cpp_exception_init_ptr->getAddress()) {
      auto thread = getCurrentThread();

      // Invoke the exception initializer
      (*p__cpp_exception_init_ptr)(&thread->_ghs__eh_globals);
   }
}

} // namespace internal

void
Module::registerGhsFunctions()
{
   RegisterKernelFunctionName("__ghsLock", ghsLock);
   RegisterKernelFunctionName("__ghsUnlock", ghsUnlock);
   RegisterKernelFunctionName("__gh_errno_ptr", ghs_errno_ptr);
   RegisterKernelFunctionName("__gh_set_errno", ghs_set_errno);
   RegisterKernelFunctionName("__gh_get_errno", ghs_get_errno);
   RegisterKernelFunctionName("__get_eh_globals", ghs_get_eh_globals);
   RegisterKernelFunctionName("__get_eh_init_block", ghs_get_eh_init_block);
   RegisterKernelFunctionName("__get_eh_mem_manage", ghs_get_eh_mem_manage);
   RegisterKernelFunctionName("__get_eh_store_globals", ghs_get_eh_store_globals);
   RegisterKernelFunctionName("__get_eh_store_globals_tdeh", ghs_get_eh_store_globals_tdeh);
   RegisterKernelFunctionName("__ghs_flock_ptr", ghs_flock_ptr);
   RegisterKernelFunctionName("__ghs_flock_file", ghs_flock_file);
   RegisterKernelFunctionName("__ghs_mtx_init", ghs_mtx_init);
   RegisterKernelFunctionName("__ghs_mtx_dst", ghs_mtx_dst);
   RegisterKernelFunctionName("__ghs_mtx_lock", ghs_mtx_lock);
   RegisterKernelFunctionName("__ghs_mtx_unlock", ghs_mtx_unlock);

   RegisterKernelDataName("__atexit_cleanup", p__atexit_cleanup);
   RegisterKernelDataName("__stdio_cleanup", p__stdio_cleanup);
   RegisterKernelDataName("__cpp_exception_init_ptr", p__cpp_exception_init_ptr);
   RegisterKernelDataName("__cpp_exception_cleanup_ptr", p__cpp_exception_cleanup_ptr);
   RegisterKernelDataName("__gh_FOPEN_MAX", p__gh_FOPEN_MAX);
   RegisterKernelDataName("_iob", p_iob);
   RegisterKernelDataName("_iob_lock", p_iob_lock);

   RegisterInternalDataName("__gh_errno", p__gh_errno);
   RegisterInternalData(ghsSpinLock);
}

void
Module::initialiseGHS()
{
   *p__gh_errno = 0;
   *p__gh_FOPEN_MAX = GHS_FOPEN_MAX;

   OSInitSpinLock(ghsSpinLock);
}

} // namespace coreinit
