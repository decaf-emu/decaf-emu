#include "coreinit.h"
#include "coreinit_ghs.h"
#include "coreinit_spinlock.h"
#include "coreinit_memheap.h"
#include "coreinit_mutex.h"
#include "interpreter.h"
#include "trace.h"

static OSSpinLock *
ghsSpinLock;

thread_local uint32_t
ghsErrno = 0;

be_wfunc_ptr<void>*
p__atexit_cleanup;

be_wfunc_ptr<void>*
p__stdio_cleanup;

be_wfunc_ptr<void, be_ptr<void>*>*
p___cpp_exception_init_ptr;

be_wfunc_ptr<void, be_ptr<void>*>*
p___cpp_exception_cleanup_ptr;

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
   if (index > *p__gh_FOPEN_MAX)
      index = *p__gh_FOPEN_MAX;
   return &(*p_iob_lock)[index];
}

uint32_t
ghs_get_errno()
{
   return ghsErrno;
}

void
ghs_set_errno(uint32_t err)
{
   ghsErrno = err;
}

void
ghs_flock_file(uint32_t lockIx)
{
   // TODO: ghs_flock_file
   assert(lockIx <= GHS_FOPEN_MAX);
   // Lock mutex for file lockIx
}

void
ghsExit(int code)
{
   // TODO: ghsExit

   tracePrint(GetCurrentFiberState(), 0, 0);

   // Must never return...
   *static_cast<uint32_t*>(0) = 0;
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

void
CoreInit::registerGhsFunctions()
{
   RegisterKernelFunctionName("__ghsLock", ghsLock);
   RegisterKernelFunctionName("__ghsUnlock", ghsUnlock);
   RegisterKernelFunctionName("__gh_set_errno", ghs_set_errno);
   RegisterKernelFunctionName("__gh_get_errno", ghs_get_errno);
   RegisterKernelFunctionName("__ghs_flock_ptr", ghs_flock_ptr);
   RegisterKernelFunctionName("__ghs_flock_file", ghs_flock_file);
   RegisterKernelFunctionName("__ghs_mtx_init", ghs_mtx_init);
   RegisterKernelFunctionName("__ghs_mtx_dst", ghs_mtx_dst);
   RegisterKernelFunctionName("__ghs_mtx_lock", ghs_mtx_lock);
   RegisterKernelFunctionName("__ghs_mtx_unlock", ghs_mtx_unlock);
   RegisterKernelFunctionName("exit", ghsExit);

   RegisterKernelDataName("__atexit_cleanup", p__atexit_cleanup);
   RegisterKernelDataName("__stdio_cleanup", p__stdio_cleanup);
   RegisterKernelDataName("___cpp_exception_init_ptr", p___cpp_exception_init_ptr);
   RegisterKernelDataName("___cpp_exception_cleanup_ptr", p___cpp_exception_cleanup_ptr);
   RegisterKernelDataName("__gh_FOPEN_MAX", p__gh_FOPEN_MAX);
   RegisterKernelDataName("_iob", p_iob);
   RegisterKernelDataName("_iob_lock", p_iob_lock);
}

void
CoreInit::initialiseGHS()
{
   *p__gh_FOPEN_MAX = GHS_FOPEN_MAX;

   ghsSpinLock = OSAllocFromSystem<OSSpinLock>();
   OSInitSpinLock(ghsSpinLock);
}
