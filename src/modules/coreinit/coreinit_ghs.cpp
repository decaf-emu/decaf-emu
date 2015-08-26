#include "coreinit.h"
#include "coreinit_ghs.h"
#include "coreinit_spinlock.h"
#include "coreinit_memheap.h"

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

uint32_t
ghs_flock_ptr()
{
   return 0;
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
   assert(false);
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
   RegisterKernelFunctionName("exit", ghsExit);

   RegisterKernelDataName("__atexit_cleanup", p__atexit_cleanup);
   RegisterKernelDataName("__stdio_cleanup", p__stdio_cleanup);
   RegisterKernelDataName("___cpp_exception_init_ptr", p___cpp_exception_init_ptr);
   RegisterKernelDataName("___cpp_exception_cleanup_ptr", p___cpp_exception_cleanup_ptr);
   RegisterKernelDataName("__gh_FOPEN_MAX", p__gh_FOPEN_MAX);
   RegisterKernelDataName("_iob", p_iob);
}

void
CoreInit::initialiseGHS()
{
   *p__gh_FOPEN_MAX = GHS_FOPEN_MAX;

   ghsSpinLock = OSAllocFromSystem<OSSpinLock>();
   OSInitSpinLock(ghsSpinLock);
}
