#pragma once
#include "coreinit_context.h"
#include "coreinit_core.h"
#include "coreinit_enum.h"
#include "coreinit_exception.h"
#include "coreinit_internal_queue.h"
#include "coreinit_time.h"

#include <cstdint>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_thread Thread
 * \ingroup coreinit
 *
 * The thread scheduler in the Wii U uses co-operative scheduling, this is different
 * to the usual pre-emptive scheduling that most operating systems use (such as
 * Windows, Linux, etc). In co-operative scheduling threads must voluntarily yield
 * execution to other threads. In pre-emptive threads are switched by the operating
 * system after an amount of time.
 *
 * With the Wii U's scheduling model the thread with the highest priority which
 * is in a non-waiting state will always be running (where 0 is the highest
 * priority and 31 is the lowest). Execution will only switch to other threads
 * once this thread has been forced to wait, such as when waiting to acquire a
 * mutex, or when the thread voluntarily yields execution to other threads which
 * have the same priority using OSYieldThread. OSYieldThread will never yield to
 * a thread with lower priority than the current thread.
 * @{
 */

#pragma pack(push, 1)

struct OSAlarm;
struct OSThread;

using OSThreadEntryPointFn = virt_func_ptr<uint32_t(uint32_t, virt_ptr<void>)>;
using OSThreadCleanupCallbackFn = virt_func_ptr<void(virt_ptr<OSThread>, virt_ptr<void>)>;
using OSThreadDeallocatorFn = virt_func_ptr<void(virt_ptr<OSThread>, virt_ptr<void>)>;

using OSContext = cafe::kernel::Context;
CHECK_SIZE(OSContext, 0x320);

struct OSMutex;

struct OSMutexQueue
{
   be2_virt_ptr<OSMutex> head;
   be2_virt_ptr<OSMutex> tail;
   be2_virt_ptr<void> parent;
   UNKNOWN(4);
};
CHECK_OFFSET(OSMutexQueue, 0x0, head);
CHECK_OFFSET(OSMutexQueue, 0x4, tail);
CHECK_OFFSET(OSMutexQueue, 0x8, parent);
CHECK_SIZE(OSMutexQueue, 0x10);

struct OSFastMutex;

struct OSFastMutexQueue
{
   be2_virt_ptr<OSFastMutex> head;
   be2_virt_ptr<OSFastMutex> tail;
};
CHECK_OFFSET(OSFastMutexQueue, 0x00, head);
CHECK_OFFSET(OSFastMutexQueue, 0x04, tail);
CHECK_SIZE(OSFastMutexQueue, 0x08);

struct OSThreadLink
{
   be2_virt_ptr<OSThread> next;
   be2_virt_ptr<OSThread> prev;
};
CHECK_OFFSET(OSThreadLink, 0x00, next);
CHECK_OFFSET(OSThreadLink, 0x04, prev);
CHECK_SIZE(OSThreadLink, 0x8);

struct OSThreadQueue
{
   be2_virt_ptr<OSThread> head;
   be2_virt_ptr<OSThread> tail;
   be2_virt_ptr<void> parent;
   UNKNOWN(4);
};
CHECK_OFFSET(OSThreadQueue, 0x00, head);
CHECK_OFFSET(OSThreadQueue, 0x04, tail);
CHECK_OFFSET(OSThreadQueue, 0x08, parent);
CHECK_SIZE(OSThreadQueue, 0x10);

struct OSThreadSimpleQueue
{
   be2_virt_ptr<OSThread> head;
   be2_virt_ptr<OSThread> tail;
};
CHECK_OFFSET(OSThreadSimpleQueue, 0x00, head);
CHECK_OFFSET(OSThreadSimpleQueue, 0x04, tail);
CHECK_SIZE(OSThreadSimpleQueue, 0x08);

struct OSTLSSection
{
   be2_virt_ptr<void> data;
   UNKNOWN(4);
};
CHECK_OFFSET(OSTLSSection, 0x00, data);
CHECK_SIZE(OSTLSSection, 0x08);

struct OSThread
{
   static constexpr uint32_t Tag = 0x74487244;

   //! Kernel thread context
   be2_struct<OSContext> context;

   //! Should always be set to the value OSThread::Tag.
   be2_val<uint32_t> tag;

   //! Bitfield of OScpu::Core
   be2_val<OSThreadState> state;

   //! Bitfield of OSThreadAttributes
   be2_val<OSThreadAttributes> attr;

   //! Unique thread ID
   be2_val<uint16_t> id;

   //! Suspend count (increased by OSSuspendThread).
   be2_val<int32_t> suspendCounter;

   //! Actual priority of thread.
   be2_val<int32_t> priority;

   //! Base priority of thread, 0 is highest priority, 31 is lowest priority.
   be2_val<int32_t> basePriority;

   //! Exit value of the thread
   be2_val<int32_t> exitValue;

   //! Core run queue stuff
   be2_virt_ptr<OSThreadQueue> coreRunQueue0;
   be2_virt_ptr<OSThreadQueue> coreRunQueue1;
   be2_virt_ptr<OSThreadQueue> coreRunQueue2;
   be2_struct<OSThreadLink> coreRunQueueLink0;
   be2_struct<OSThreadLink> coreRunQueueLink1;
   be2_struct<OSThreadLink> coreRunQueueLink2;

   //! Queue the thread is currently waiting on
   be2_virt_ptr<OSThreadQueue> queue;

   //! Link used for thread queue
   be2_struct<OSThreadLink> link;

   //! Queue of threads waiting to join this thread
   be2_struct<OSThreadQueue> joinQueue;

   //! Mutex this thread is waiting to lock
   be2_virt_ptr<OSMutex> mutex;

   //! Queue of mutexes this thread owns
   be2_struct<OSMutexQueue> mutexQueue;

   //! Link for global active thread queue
   be2_struct<OSThreadLink> activeLink;

   //! Stack start (top, highest address)
   be2_virt_ptr<uint32_t> stackStart;

   //! Stack end (bottom, lowest address)
   be2_virt_ptr<uint32_t> stackEnd;

   //! Thread entry point set in OSCreateThread
   be2_val<OSThreadEntryPointFn> entryPoint;

   UNKNOWN(0x408 - 0x3a0);

   //! GHS Exception handling thread-specifics
   be2_virt_ptr<void> eh_globals;
   be2_array<virt_ptr<void>, 9> eh_mem_manage;
   be2_array<virt_ptr<void>, 6> eh_store_globals;
   be2_array<virt_ptr<void>, 76> eh_store_globals_tdeh;

   be2_val<BOOL> alarmCancelled;

   //! Thread specific values, accessed with OSSetThreadSpecific and OSGetThreadSpecific.
   be2_array<uint32_t, 16> specific;

   //! Thread type
   be2_val<OSThreadType> type;

   //! Thread name, accessed with OSSetThreadName and OSGetThreadName.
   be2_virt_ptr<const char> name;

   //! Alarm the thread is waiting on in OSWaitEventWithTimeout
   be2_virt_ptr<OSAlarm> waitEventTimeoutAlarm;

   //! The stack pointer passed in OSCreateThread.
   be2_virt_ptr<uint32_t> userStackPointer;

   //! Called just before thread is terminated, set with OSSetThreadCleanupCallback
   be2_val<OSThreadCleanupCallbackFn> cleanupCallback;

   //! Called just after a thread is terminated, set with OSSetThreadDeallocator
   be2_val<OSThreadDeallocatorFn> deallocator;

   //! Current thread cancel state, controls whether the thread is allowed to cancel or not
   be2_val<OSThreadCancelState> cancelState;

   //! Current thread request, used for cancelleing and suspending the thread.
   be2_val<OSThreadRequest> requestFlag;

   //! Pending suspend request count
   be2_val<int32_t> needSuspend;

   //! Result of thread suspend
   be2_val<int32_t> suspendResult;

   //! Queue of threads waiting for a thread to be suspended.
   be2_struct<OSThreadQueue> suspendQueue;

   UNKNOWN(0x4);

   //! How many ticks the thread should run for before suspension.
   be2_val<int64_t> runQuantumTicks;

   //! The total amount of core time consumed by this thread (Does not include time while Running)
   be2_val<uint64_t> coreTimeConsumedNs;

   //! The number of times this thread has been awoken.
   be2_val<uint64_t> wakeCount;

   be2_val<OSTime> unk0x610;
   be2_val<OSTime> unk0x618;
   be2_val<OSTime> unk0x620;
   be2_val<OSTime> unk0x628;

   //! Callback for DSI exception
   be2_array<OSExceptionCallbackFn, OSGetCoreCount()> dsiCallback;

   //! Callback for ISI exception
   be2_array<OSExceptionCallbackFn, OSGetCoreCount()> isiCallback;

   //! Callback for Program exception
   be2_array<OSExceptionCallbackFn, OSGetCoreCount()> programCallback;

   //! Callback for PerfMon exception
   be2_array<OSExceptionCallbackFn, OSGetCoreCount()> perfMonCallback;

   UNKNOWN(0x4);

   //! Number of TLS sections
   be2_val<uint16_t> tlsSectionCount;

   UNKNOWN(0x2);

   //! TLS Sections
   be2_virt_ptr<OSTLSSection> tlsSections;

   //! The fast mutex we are currently waiting for
   be2_virt_ptr<OSFastMutex> fastMutex;

   //! The fast mutexes we are currently contended on
   be2_struct<OSFastMutexQueue> contendedFastMutexes;

   //! The fast mutexes we currently own locks on
   be2_struct<OSFastMutexQueue> fastMutexQueue;

   //! Callback for Alignment exception
   be2_array<OSExceptionCallbackFn, OSGetCoreCount()> alignCallback;

   UNKNOWN(0x6A0 - 0x68C);
};
CHECK_OFFSET(OSThread, 0x320, tag);
CHECK_OFFSET(OSThread, 0x324, state);
CHECK_OFFSET(OSThread, 0x325, attr);
CHECK_OFFSET(OSThread, 0x326, id);
CHECK_OFFSET(OSThread, 0x328, suspendCounter);
CHECK_OFFSET(OSThread, 0x32c, priority);
CHECK_OFFSET(OSThread, 0x330, basePriority);
CHECK_OFFSET(OSThread, 0x334, exitValue);
CHECK_OFFSET(OSThread, 0x338, coreRunQueue0);
CHECK_OFFSET(OSThread, 0x33C, coreRunQueue1);
CHECK_OFFSET(OSThread, 0x340, coreRunQueue2);
CHECK_OFFSET(OSThread, 0x344, coreRunQueueLink0);
CHECK_OFFSET(OSThread, 0x34C, coreRunQueueLink1);
CHECK_OFFSET(OSThread, 0x354, coreRunQueueLink2);
CHECK_OFFSET(OSThread, 0x35C, queue);
CHECK_OFFSET(OSThread, 0x360, link);
CHECK_OFFSET(OSThread, 0x368, joinQueue);
CHECK_OFFSET(OSThread, 0x378, mutex);
CHECK_OFFSET(OSThread, 0x37C, mutexQueue);
CHECK_OFFSET(OSThread, 0x38C, activeLink);
CHECK_OFFSET(OSThread, 0x394, stackStart);
CHECK_OFFSET(OSThread, 0x398, stackEnd);
CHECK_OFFSET(OSThread, 0x39C, entryPoint);
CHECK_OFFSET(OSThread, 0x408, eh_globals);
CHECK_OFFSET(OSThread, 0x40C, eh_mem_manage);
CHECK_OFFSET(OSThread, 0x430, eh_store_globals);
CHECK_OFFSET(OSThread, 0x448, eh_store_globals_tdeh);
CHECK_OFFSET(OSThread, 0x578, alarmCancelled);
CHECK_OFFSET(OSThread, 0x57C, specific);
CHECK_OFFSET(OSThread, 0x5BC, type);
CHECK_OFFSET(OSThread, 0x5C0, name);
CHECK_OFFSET(OSThread, 0x5C4, waitEventTimeoutAlarm);
CHECK_OFFSET(OSThread, 0x5C8, userStackPointer);
CHECK_OFFSET(OSThread, 0x5CC, cleanupCallback);
CHECK_OFFSET(OSThread, 0x5D0, deallocator);
CHECK_OFFSET(OSThread, 0x5D4, cancelState);
CHECK_OFFSET(OSThread, 0x5D8, requestFlag);
CHECK_OFFSET(OSThread, 0x5DC, needSuspend);
CHECK_OFFSET(OSThread, 0x5E0, suspendResult);
CHECK_OFFSET(OSThread, 0x5E4, suspendQueue);
CHECK_OFFSET(OSThread, 0x5F8, runQuantumTicks);
CHECK_OFFSET(OSThread, 0x600, coreTimeConsumedNs);
CHECK_OFFSET(OSThread, 0x608, wakeCount);
CHECK_OFFSET(OSThread, 0x610, unk0x610);
CHECK_OFFSET(OSThread, 0x618, unk0x618);
CHECK_OFFSET(OSThread, 0x620, unk0x620);
CHECK_OFFSET(OSThread, 0x628, unk0x628);
CHECK_OFFSET(OSThread, 0x630, dsiCallback);
CHECK_OFFSET(OSThread, 0x63C, isiCallback);
CHECK_OFFSET(OSThread, 0x648, programCallback);
CHECK_OFFSET(OSThread, 0x654, perfMonCallback);
CHECK_OFFSET(OSThread, 0x664, tlsSectionCount);
CHECK_OFFSET(OSThread, 0x668, tlsSections);
CHECK_OFFSET(OSThread, 0x66C, fastMutex);
CHECK_OFFSET(OSThread, 0x670, contendedFastMutexes);
CHECK_OFFSET(OSThread, 0x678, fastMutexQueue);
CHECK_OFFSET(OSThread, 0x680, alignCallback);
CHECK_SIZE(OSThread, 0x6A0);

#pragma pack(pop)

void
OSCancelThread(virt_ptr<OSThread> thread);

int32_t
OSCheckActiveThreads();

int32_t
OSCheckThreadStackUsage(virt_ptr<OSThread> thread);

void
OSClearStack(uint32_t value);

void
OSClearThreadStackUsage(virt_ptr<OSThread> thread);

void
OSContinueThread(virt_ptr<OSThread> thread);

BOOL
OSCreateThread(virt_ptr<OSThread> thread,
               OSThreadEntryPointFn entry,
               uint32_t argc,
               virt_ptr<void> argv,
               virt_ptr<uint32_t> stackTop,
               uint32_t stackSize,
               int32_t priority,
               OSThreadAttributes attributes);

BOOL
OSCreateThreadType(virt_ptr<OSThread> thread,
                   OSThreadEntryPointFn entry,
                   uint32_t argc,
                   virt_ptr<void> argv,
                   virt_ptr<uint32_t> stackTop,
                   uint32_t stackSize,
                   int32_t priority,
                   OSThreadAttributes attributes,
                   OSThreadType type);

BOOL
coreinit__OSCreateThreadType(virt_ptr<OSThread> thread,
                             OSThreadEntryPointFn entry,
                             uint32_t argc,
                             virt_ptr<void> argv,
                             virt_ptr<uint32_t> stackTop,
                             uint32_t stackSize,
                             int32_t priority,
                             OSThreadAttributes attributes,
                             OSThreadType type);

void
OSDetachThread(virt_ptr<OSThread> thread);

void
OSExitThread(int value);

void
OSGetActiveThreadLink(virt_ptr<OSThread> thread,
                      virt_ptr<OSThreadLink> link);

virt_ptr<OSThread>
OSGetCurrentThread();

virt_ptr<OSThread>
OSGetDefaultThread(uint32_t coreID);

virt_ptr<uint32_t>
OSGetStackPointer();

virt_ptr<uint32_t>
OSGetUserStackPointer(virt_ptr<OSThread> thread);

uint32_t
OSGetThreadAffinity(virt_ptr<OSThread> thread);

virt_ptr<const char>
OSGetThreadName(virt_ptr<OSThread> thread);

int32_t
OSGetThreadPriority(virt_ptr<OSThread> thread);

uint32_t
OSGetThreadSpecific(uint32_t id);

void
OSInitThreadQueue(virt_ptr<OSThreadQueue> queue);

void
OSInitThreadQueueEx(virt_ptr<OSThreadQueue> queue,
                    virt_ptr<void> parent);

BOOL
OSIsThreadSuspended(virt_ptr<OSThread> thread);

BOOL
OSIsThreadTerminated(virt_ptr<OSThread> thread);

BOOL
OSJoinThread(virt_ptr<OSThread> thread,
             virt_ptr<int32_t> exitValue);

void
OSPrintCurrentThreadState();

int32_t
OSResumeThread(virt_ptr<OSThread> thread);

BOOL
OSRunThread(virt_ptr<OSThread> thread,
            OSThreadEntryPointFn entry,
            uint32_t argc,
            virt_ptr<void> argv);

BOOL
OSSetThreadAffinity(virt_ptr<OSThread> thread,
                    uint32_t affinity);

BOOL
OSSetThreadCancelState(BOOL state);

OSThreadCleanupCallbackFn
OSSetThreadCleanupCallback(virt_ptr<OSThread> thread,
                           OSThreadCleanupCallbackFn callback);
OSThreadDeallocatorFn
OSSetThreadDeallocator(virt_ptr<OSThread> thread,
                       OSThreadDeallocatorFn deallocator);

void
OSSetThreadName(virt_ptr<OSThread> thread,
                virt_ptr<const char> name);

BOOL
OSSetThreadPriority(virt_ptr<OSThread> thread,
                    int32_t priority);

BOOL
OSSetThreadRunQuantum(virt_ptr<OSThread> thread,
                      uint32_t quantumUS);

void
OSSetThreadSpecific(uint32_t id,
                    uint32_t value);

BOOL
OSSetThreadStackUsage(virt_ptr<OSThread> thread);

void
OSSleepThread(virt_ptr<OSThreadQueue> queue);

void
OSSleepTicks(OSTime ticks);

uint32_t
OSSuspendThread(virt_ptr<OSThread> thread);

void
OSTestThreadCancel();

void
OSWakeupThread(virt_ptr<OSThreadQueue> queue);

void
OSYieldThread();

/** @} */

namespace internal
{

void
initialiseThreads();

void
setUserStackPointer(virt_ptr<uint32_t> stack);

void
removeUserStackPointer(virt_ptr<uint32_t> stack);

uint32_t
pinThreadAffinity();

void
unpinThreadAffinity(uint32_t affinity);

void
queueThreadDeallocation(virt_ptr<OSThread> thread);

void
exitThreadNoLock(int32_t value);

struct ThreadIsLess
{
   bool operator()(virt_ptr<OSThread> lhs,
                   virt_ptr<OSThread> rhs) const
   {
      return lhs->priority <= rhs->priority;
   }

};

using ThreadQueue = SortedQueue<OSThreadQueue, OSThreadLink, OSThread, &OSThread::link, ThreadIsLess>;

} // namespace internal

} // namespace coreinit
