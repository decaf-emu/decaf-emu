#include "coreinit.h"
#include "coreinit_ghs.h"
#include "coreinit_spinlock.h"
#include "coreinit_memory.h"

p32<OSSpinLock> ghsSpinLock;

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
   RegisterSystemFunctionName("__ghsLock", ghsLock);
   RegisterSystemFunctionName("__ghsUnlock", ghsUnlock);
}

void
CoreInit::initialiseGHS()
{
   ghsSpinLock = OSAllocFromSystem(sizeof(OSSpinLock), 4);
   OSInitSpinLock(ghsSpinLock);
}
