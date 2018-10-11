#include "coreinit.h"
#include "coreinit_cosreport.h"
#include "coreinit_driver.h"
#include "coreinit_memdefaultheap.h"
#include "coreinit_mutex.h"
#include "coreinit_spinlock.h"
#include "coreinit_interrupts.h"
#include "coreinit_osreport.h"

#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/kernel/cafe_kernel_process.h"

#include <common/bitfield.h>
#include <libcpu/cpu.h>

namespace cafe::coreinit
{

using AtExitFn = virt_func_ptr<void(void)>;
using AtExitCleanupFn = virt_func_ptr<void(int32_t)>;
using StdioCleanupFn = virt_func_ptr<void(void)>;
using CppExceptionInitPtrFn = virt_func_ptr<void(virt_ptr<virt_ptr<void>>)>;
using CppExceptionCleanupPtrFn = virt_func_ptr<void(virt_ptr<void>)>;

constexpr auto GHS_FLOCK_MAX = 100u;
constexpr auto GHS_FOPEN_MAX = uint16_t { 20u };

struct ghs_atexit
{
   be2_val<AtExitFn> callback;
   be2_virt_ptr<ghs_atexit> next;
};
CHECK_OFFSET(ghs_atexit, 0x0, callback);
CHECK_OFFSET(ghs_atexit, 0x4, next);
CHECK_SIZE(ghs_atexit, 8);

BITFIELD_BEG(ghs_iobuf_bits, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, readwrite);
   BITFIELD_ENTRY(1, 1, bool, writable);
   BITFIELD_ENTRY(2, 1, bool, readable);
   BITFIELD_ENTRY(18, 14, uint32_t, channel);
BITFIELD_END

struct ghs_iobuf
{
   be2_virt_ptr<uint8_t> nextPtr;
   be2_virt_ptr<uint8_t> basePtr;
   be2_val<int32_t> bytesLeft;
   be2_val<ghs_iobuf_bits> info;
};
CHECK_OFFSET(ghs_iobuf, 0x0, nextPtr);
CHECK_OFFSET(ghs_iobuf, 0x4, basePtr);
CHECK_OFFSET(ghs_iobuf, 0x8, bytesLeft);
CHECK_OFFSET(ghs_iobuf, 0xc, info);
CHECK_SIZE(ghs_iobuf, 0x10);

struct StaticGhsData
{
   be2_struct<OSSpinLock> ghsLock;
   be2_virt_ptr<ghs_atexit> atExitCallbacks;

   be2_struct<OSMutex> flockMutex;
   be2_val<uint32_t> freeFlockIdx;
   be2_array<uint8_t, GHS_FLOCK_MAX> flockInUse;
   be2_array<OSMutex, GHS_FLOCK_MAX> flocks;
};

static virt_ptr<StaticGhsData>
sGhsData = nullptr;

//! __atexit_cleanup
static virt_ptr<AtExitCleanupFn> atexit_cleanup = nullptr;

//! __stdio_cleanup
static virt_ptr<StdioCleanupFn> stdio_cleanup = nullptr;

//! __cpp_exception_init_ptr
static virt_ptr<CppExceptionInitPtrFn> cpp_exception_init_ptr = nullptr;

//! __cpp_exception_cleanup_ptr
static virt_ptr<CppExceptionCleanupPtrFn> cpp_exception_cleanup_ptr = nullptr;

//! __ghs_cpp_locks
static virt_ptr<OSMutex> ghs_cpp_locks = nullptr;

//! __gh_FOPEN_MAX
static virt_ptr<uint16_t> gh_FOPEN_MAX = nullptr;

//! _iob
static virt_ptr<ghs_iobuf[GHS_FOPEN_MAX]> ghs_iob = nullptr;

//! _iob_lock
static virt_ptr<uint32_t[GHS_FOPEN_MAX + 1]> ghs_iob_lock = nullptr;

//! errno
static virt_ptr<int32_t> ghs_errno = nullptr;

//! environ
static virt_ptr<virt_ptr<void>> ghs_environ = nullptr;

/**
 * __ghsLock
 */
void
ghsLock()
{
   OSUninterruptibleSpinLock_Acquire(virt_addrof(sGhsData->ghsLock));
}


/**
 * __ghsUnlock
 */
void
ghsUnlock()
{
   OSUninterruptibleSpinLock_Release(virt_addrof(sGhsData->ghsLock));
}


/**
 * __ghs_at_exit
 */
void
ghs_at_exit(virt_ptr<ghs_atexit> atExitCallback)
{
   ghsLock();
   atExitCallback->next = sGhsData->atExitCallbacks;
   sGhsData->atExitCallbacks = atExitCallback;
   ghsUnlock();
}


/**
 * __ghs_at_exit_cleanup
 */
void
ghs_at_exit_cleanup()
{
   for (auto item = sGhsData->atExitCallbacks; item; item = item->next) {
      cafe::invoke(cpu::this_core::state(),
                   item->callback);
   }
}

/**
 * __PPCExit
 */
void
ghs_PPCExit(int32_t code)
{
   internal::driverOnDone();
   kernel::exitProcess(code);
}

/**
 * _Exit
 */
void
ghs_Exit(int32_t code)
{
   ghs_at_exit_cleanup();
   ghs_PPCExit(code);
}

/**
 * exit
 */
void
ghs_exit(int32_t code)
{
   internal::COSVerbose(COSReportModule::Unknown1, "ATEXIT: RPX (calls RPX DTORs)");

   if (*atexit_cleanup) {
      cafe::invoke(cpu::this_core::state(),
                   *atexit_cleanup,
                   code);
   }

   if (*stdio_cleanup) {
      cafe::invoke(cpu::this_core::state(),
                   *stdio_cleanup);
   }

   ghs_Exit(code);
}


/**
 * __gh_errno_ptr
 */
virt_ptr<int32_t>
gh_errno_ptr()
{
   auto thread = OSGetCurrentThread();
   if (!thread) {
      return ghs_errno;
   } else {
      return virt_addrof(thread->context.error);
   }
}


/**
 * __gh_get_errno
 */
int32_t
gh_get_errno()
{
   return *gh_errno_ptr();
}


/**
 * __gh_set_errno
 */
void
gh_set_errno(int32_t error)
{
   *gh_errno_ptr() = error;
}


/**
 * __ghs_flock_create
 */
void
ghs_flock_create(virt_ptr<uint32_t> outFlockIdx)
{
   auto flockIdx = sGhsData->freeFlockIdx;
   OSLockMutex(virt_addrof(sGhsData->flockMutex));

   if (flockIdx >= 101 || sGhsData->flockInUse[flockIdx]) {
      for (flockIdx = 0u; flockIdx < sGhsData->flockInUse.size(); ++flockIdx) {
         if (!sGhsData->flockInUse[flockIdx]) {
            break;
         }
      }

      if (flockIdx >= sGhsData->flockInUse.size()) {
         internal::OSPanic("locks.c", 122, "All flocks are in use");
      }
   }

   OSInitMutex(virt_addrof(sGhsData->flocks[flockIdx]));
   sGhsData->flockInUse[flockIdx] = uint8_t { 1 };
   *outFlockIdx = flockIdx;
   sGhsData->freeFlockIdx = flockIdx + 1;
   OSUnlockMutex(virt_addrof(sGhsData->flockMutex));
}


/**
 * __ghs_flock_destroy
 */
void
ghs_flock_destroy(uint32_t flockIdx)
{
   sGhsData->flockInUse[flockIdx] = uint8_t { 0 };
   sGhsData->freeFlockIdx = flockIdx;
}


/**
 * __ghs_flock_file
 */
void
ghs_flock_file(uint32_t flockIdx)
{
   auto mutex = virt_addrof(sGhsData->flocks[flockIdx]);
   if (!OSIsInterruptEnabled()) {
      if (mutex->owner && mutex->owner != OSGetCurrentThread()) {
         internal::COSWarn(COSReportModule::Unknown1,
                           "***\n"
                           "*** STD LIBC FILE I/O:\n"
                           "Locking a mutex owned by another thread while interrupts are off!!\n"
                           "***\n");
      }
   }

   OSLockMutex(mutex);
}


/**
 * __ghs_flock_ptr
 */
virt_ptr<uint32_t>
ghs_flock_ptr(virt_ptr<ghs_iobuf> iob)
{
   auto index = static_cast<uint32_t>(iob - virt_addrof(ghs_iob->at(0)));
   if (index > *gh_FOPEN_MAX) {
      index = *gh_FOPEN_MAX;
   }

   return virt_addrof(ghs_iob_lock->at(index));
}


/**
 * __ghs_ftrylock_file
 */
int32_t
ghs_ftrylock_file(uint32_t flockIdx)
{
   if (OSTryLockMutex(virt_addrof(sGhsData->flocks[flockIdx]))) {
      return 0;
   } else {
      return 1234;
   }
}


/**
 * __gh_iob_init
 */
void
gh_iob_init()
{
   // stdin
   ghs_iob->at(0).info = ghs_iob->at(0).info.value()
      .readable(true)
      .channel(0);

   if (auto flock_ptr = ghs_flock_ptr(virt_addrof(ghs_iob->at(0)))) {
      ghs_flock_create(flock_ptr);
   }

   // stdout
   ghs_iob->at(1).info = ghs_iob->at(1).info.value()
      .writable(true)
      .channel(1);

   if (auto flock_ptr = ghs_flock_ptr(virt_addrof(ghs_iob->at(1)))) {
      ghs_flock_create(flock_ptr);
   }

   // stderr
   ghs_iob->at(2).info = ghs_iob->at(2).info.value()
      .writable(true)
      .channel(2);

   if (auto flock_ptr = ghs_flock_ptr(virt_addrof(ghs_iob->at(2)))) {
      ghs_flock_create(flock_ptr);
   }
}


/**
 * __gh_lock_init
 */
void
gh_lock_init()
{
   sGhsData->flockInUse.fill(0);
   sGhsData->freeFlockIdx = 0u;

   OSInitSpinLock(virt_addrof(sGhsData->ghsLock));
   OSInitMutex(virt_addrof(sGhsData->flockMutex));
   OSInitMutex(ghs_cpp_locks);
}


/**
 * __ghs_funlock_file
 */
void
ghs_funlock_file(uint32_t flockIdx)
{
   OSUnlockMutex(virt_addrof(sGhsData->flocks[flockIdx]));
}


/**
 * __ghs_mtx_init
 */
void
ghs_mtx_init(virt_ptr<virt_ptr<OSMutex>> outMutex)
{
   auto mutex = virt_cast<OSMutex *>(MEMAllocFromDefaultHeapEx(sizeof(OSMutex), 8));
   OSInitMutex(mutex);
   *outMutex = mutex;
}


/**
 * __ghs_mtx_dst
 */
void
ghs_mtx_dst(virt_ptr<virt_ptr<OSMutex>> mutex)
{
   MEMFreeToDefaultHeap(*mutex);
}


/**
 * __ghs_mtx_lock
 */
void
ghs_mtx_lock(virt_ptr<virt_ptr<OSMutex>> mutex)
{
   OSLockMutex(*mutex);
}


/**
 * __ghs_mtx_unlock
 */
void
ghs_mtx_unlock(virt_ptr<virt_ptr<OSMutex>> mutex)
{
   OSUnlockMutex(*mutex);
}


/**
 * __get_eh_globals
 */
virt_ptr<void>
get_eh_globals()
{
   return OSGetCurrentThread()->eh_globals;
}


/**
 * __get_eh_init_block
 */
virt_ptr<void>
get_eh_init_block()
{
   return nullptr;
}


/**
 * __get_eh_mem_manage
 */
virt_ptr<void>
get_eh_mem_manage()
{
   return virt_addrof(OSGetCurrentThread()->eh_mem_manage);
}


/**
 * __get_eh_store_globals
 */
virt_ptr<void>
get_eh_store_globals()
{
   return virt_addrof(OSGetCurrentThread()->eh_store_globals);
}


/**
 * __get_eh_store_globals_tdeh
 */
virt_ptr<void>
get_eh_store_globals_tdeh()
{
   return virt_addrof(OSGetCurrentThread()->eh_store_globals_tdeh);
}


namespace internal
{

void
initialiseGhs()
{
   *gh_FOPEN_MAX = GHS_FOPEN_MAX;
   gh_lock_init();
   gh_iob_init();
}

void
ghsExceptionInit(virt_ptr<OSThread> thread)
{
   if (*cpp_exception_init_ptr) {
      cafe::invoke(cpu::this_core::state(),
                   *cpp_exception_init_ptr,
                   virt_addrof(thread->eh_globals));
   }
}

void
ghsExceptionCleanup(virt_ptr<OSThread> thread)
{
   if (*cpp_exception_cleanup_ptr && thread->eh_globals) {
      cafe::invoke(cpu::this_core::state(),
                   *cpp_exception_cleanup_ptr,
                   thread->eh_globals);
   }
}

} // namespace internal

void
Library::registerGhsSymbols()
{

   RegisterFunctionExportName("__ghsLock", ghsLock);
   RegisterFunctionExportName("__ghsUnlock", ghsUnlock);

   RegisterFunctionExportName("__ghs_at_exit", ghs_at_exit);
   RegisterFunctionExportName("__ghs_at_exit_cleanup", ghs_at_exit_cleanup);
   RegisterFunctionExportName("exit", ghs_exit);
   RegisterFunctionExportName("_Exit", ghs_Exit);
   RegisterFunctionExportName("__PPCExit", ghs_PPCExit);

   RegisterFunctionExportName("__gh_errno_ptr", gh_errno_ptr);
   RegisterFunctionExportName("__gh_get_errno", gh_get_errno);
   RegisterFunctionExportName("__gh_set_errno", gh_set_errno);

   RegisterFunctionExportName("__gh_iob_init", gh_iob_init);
   RegisterFunctionExportName("__gh_lock_init", gh_lock_init);

   RegisterFunctionExportName("__ghs_flock_create", ghs_flock_create);
   RegisterFunctionExportName("__ghs_flock_destroy", ghs_flock_destroy);
   RegisterFunctionExportName("__ghs_flock_file", ghs_flock_file);
   RegisterFunctionExportName("__ghs_flock_ptr", ghs_flock_ptr);
   RegisterFunctionExportName("__ghs_ftrylock_file", ghs_ftrylock_file);
   RegisterFunctionExportName("__ghs_funlock_file", ghs_funlock_file);

   RegisterFunctionExportName("__ghs_mtx_init", ghs_mtx_init);
   RegisterFunctionExportName("__ghs_mtx_dst", ghs_mtx_dst);
   RegisterFunctionExportName("__ghs_mtx_lock", ghs_mtx_lock);
   RegisterFunctionExportName("__ghs_mtx_unlock", ghs_mtx_unlock);

   RegisterFunctionExportName("__get_eh_globals", get_eh_globals);
   RegisterFunctionExportName("__get_eh_init_block", get_eh_init_block);
   RegisterFunctionExportName("__get_eh_mem_manage", get_eh_mem_manage);
   RegisterFunctionExportName("__get_eh_store_globals", get_eh_store_globals);
   RegisterFunctionExportName("__get_eh_store_globals_tdeh", get_eh_store_globals_tdeh);

   RegisterDataExportName("__atexit_cleanup", atexit_cleanup);
   RegisterDataExportName("__stdio_cleanup", stdio_cleanup);
   RegisterDataExportName("__cpp_exception_init_ptr", cpp_exception_init_ptr);
   RegisterDataExportName("__cpp_exception_cleanup_ptr", cpp_exception_cleanup_ptr);
   RegisterDataExportName("__ghs_cpp_locks", ghs_cpp_locks);
   RegisterDataExportName("__gh_FOPEN_MAX", gh_FOPEN_MAX);
   RegisterDataExportName("_iob", ghs_iob);
   RegisterDataExportName("_iob_lock", ghs_iob_lock);
   RegisterDataExportName("errno", ghs_errno);
   RegisterDataExportName("environ", ghs_environ);

   RegisterDataInternal(sGhsData);
}

} // namespace cafe::coreinit
