#include "coreinit.h"
#include "coreinit_ghs.h"
#include "coreinit_spinlock.h"
#include "coreinit_memheap.h"

static const auto GHS_FOPEN_MAX = 0x64;

static OSSpinLock *
ghsSpinLock;

thread_local uint32_t
ghsErrno = 0;

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
   assert(lockIx <= GHS_FOPEN_MAX);
   // Lock mutex for file lockIx
}

/*
.dimport __atexit_cleanup
.dimport __cpp_exception_cleanup_ptr
.dimport __cpp_exception_init_ptr
.dimport __gh_FOPEN_MAX
.dimport _iob
*/

void
CoreInit::registerGhsFunctions()
{
   RegisterKernelFunctionName("__ghsLock", ghsLock);
   RegisterKernelFunctionName("__ghsUnlock", ghsUnlock);
   RegisterKernelFunctionName("__gh_set_errno", ghs_set_errno);
   RegisterKernelFunctionName("__gh_get_errno", ghs_get_errno);
   RegisterKernelFunctionName("__ghs_flock_ptr", ghs_flock_ptr);
   RegisterKernelFunctionName("__ghs_flock_file", ghs_flock_file);
}

void
CoreInit::initialiseGHS()
{
   ghsSpinLock = OSAllocFromSystem<OSSpinLock>();
   OSInitSpinLock(ghsSpinLock);
}
