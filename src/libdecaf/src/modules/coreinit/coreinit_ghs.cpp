#include "coreinit.h"
#include "coreinit_ghs.h"
#include "coreinit_interrupts.h"
#include "coreinit_spinlock.h"
#include "coreinit_memheap.h"
#include "coreinit_mutex.h"
#include "coreinit_scheduler.h"
#include "coreinit_time.h"
#include "kernel/kernel_loader.h"
#include "libcpu/trace.h"
#include "ppcutils/wfunc_call.h"
#include "common/decaf_assert.h"
#include "common/log.h"
#include <array>

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

static be_ptr<char>*
p_environEmpty;

static be_ptr<be_ptr<char>>*
p_environ;

static std::array<_ghs_iobuf*, GHS_FOPEN_MAX>
p_iob;

static std::array<be_val<uint32_t>*, GHS_FOPEN_MAX + 1>
p_iob_lock;

static std::array<OSMutex*, GHS_FLOCK_MAX>
p_iob_mutexes;

static std::array<bool*, GHS_FLOCK_MAX>
p_iob_mutex_used;

static wfunc_ptr<int32_t, uint32_t>
sSyscallFunc;

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

void
ghs_flock_create(be_val<uint32_t> *lock)
{
   for (auto i = 0; i < GHS_FLOCK_MAX; ++i) {
      if (!*p_iob_mutex_used[i]) {
         *lock = i;
         OSInitMutex(p_iob_mutexes[i]);
         *p_iob_mutex_used[i] = true;
         return;
      }
   }

   *lock = -1;
   gLog->warn("ghs_flock_create ran out of locks!");
}

void
ghs_flock_destroy(uint32_t lock)
{
   decaf_check(lock < GHS_FLOCK_MAX);
   *p_iob_mutex_used[lock] = false;
}

void
ghs_flock_file(uint32_t lock)
{
   decaf_check(lock <= GHS_FOPEN_MAX);
   OSLockMutex(p_iob_mutexes[lock]);
}

BOOL
ghs_ftrylock_file(uint32_t lock)
{
   decaf_check(lock <= GHS_FOPEN_MAX);
   return OSTryLockMutex(p_iob_mutexes[lock]);
}

void
ghs_funlock_file(uint32_t lock)
{
   decaf_check(lock <= GHS_FOPEN_MAX);
   OSUnlockMutex(p_iob_mutexes[lock]);
}

be_val<uint32_t>*
ghs_flock_ptr(void *file)
{
   auto index = static_cast<size_t>(reinterpret_cast<_ghs_iobuf*>(file) - p_iob[0]);

   if (index > *p__gh_FOPEN_MAX) {
      index = *p__gh_FOPEN_MAX;
   }

   return p_iob_lock[index];
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

void ghs_mtx_init(void *mtx)
{
   auto pmutex = static_cast<be_ptr<OSMutex>*>(mtx);
   *pmutex = static_cast<OSMutex*>((*pMEMAllocFromDefaultHeapEx)(sizeof(OSMutex), 8));
   OSInitMutex(*pmutex);
}

void ghs_mtx_dst(void *mtx)
{
   auto pmutex = static_cast<be_ptr<OSMutex>*>(mtx);
   (*pMEMFreeToDefaultHeap)(*pmutex);
   *pmutex = nullptr;
}

void ghs_mtx_lock(void *mtx)
{
   auto pmutex = static_cast<be_ptr<OSMutex>*>(mtx);
   OSLockMutex(*pmutex);
}

void ghs_mtx_unlock(void *mtx)
{
   auto pmutex = static_cast<be_ptr<OSMutex>*>(mtx);
   OSUnlockMutex(*pmutex);
}

static int32_t
syscall_time()
{
   auto t = internal::toTimepoint(OSGetTime());
   return t.time_since_epoch() / std::chrono::seconds(1);
}

static int32_t
syscall(uint32_t id)
{
   switch (id) {
   case 0xe:
      return syscall_time();
   default:
      decaf_abort(fmt::format("Unsupported ghs syscalls {:x}", id));
   }
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
   RegisterKernelFunctionName("__ghs_flock_create", ghs_flock_create);
   RegisterKernelFunctionName("__ghs_flock_destroy", ghs_flock_destroy);
   RegisterKernelFunctionName("__ghs_flock_file", ghs_flock_file);
   RegisterKernelFunctionName("__ghs_ftrylock_file", ghs_ftrylock_file);
   RegisterKernelFunctionName("__ghs_funlock_file", ghs_funlock_file);

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
   RegisterKernelDataName("environ", p_environ);

   RegisterInternalFunction(syscall, sSyscallFunc);
   RegisterInternalDataName("__gh_errno", p__gh_errno);
   RegisterInternalData(ghsSpinLock);
   RegisterInternalData(p_iob_mutexes);
   RegisterInternalData(p_iob_mutex_used);
   RegisterInternalData(p_environEmpty);
}

void
Module::initialiseGHS()
{
   *p__gh_errno = 0;
   *p__gh_FOPEN_MAX = GHS_FOPEN_MAX;

   *p_environEmpty = nullptr;
   *p_environ = p_environEmpty;

   OSInitSpinLock(ghsSpinLock);

   // This is actually meant to be during _crt_startup
   p_iob[0]->info = static_cast<_ghs_iobuf_bits>(p_iob[0]->info)
      .readable().set(true)
      .channel().set(0);
   p_iob[1]->info = static_cast<_ghs_iobuf_bits>(p_iob[1]->info)
      .writable().set(true)
      .channel().set(1);
   p_iob[2]->info = static_cast<_ghs_iobuf_bits>(p_iob[2]->info)
      .writable().set(true)
      .channel().set(2);

   ghs_flock_create(p_iob_lock[0]);
   ghs_flock_create(p_iob_lock[1]);
   ghs_flock_create(p_iob_lock[2]);

   kernel::loader::setSyscallAddress(sSyscallFunc);
}

} // namespace coreinit
