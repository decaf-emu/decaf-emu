#pragma once
#include "systemtypes.h"
#include "coreinit_time.h"
#include "coreinit_threadqueue.h"

#pragma pack(push, 1)

struct Fiber;

struct OSContext
{
   static const uint64_t Tag1 = 0x4F53436F6E747874ull;
   be_val<uint64_t> tag;
   be_val<uint32_t> gpr[32];
   be_val<uint32_t> cr;
   be_val<uint32_t> lr;
   be_val<uint32_t> ctr;
   be_val<uint32_t> xer;
   be_val<uint32_t> srr0;
   be_val<uint32_t> srr1;
   UNKNOWN(0x14);
   be_val<uint32_t> fpscr;
   be_val<double> fpr[32];
   be_val<uint16_t> spinLockCount;
   be_val<uint16_t> state;
   be_val<uint32_t> gqr[8];
   UNKNOWN(4);
   be_val<double> psf[32];
   be_val<uint64_t> coretime[3];
   be_val<uint64_t> starttime;
   be_val<uint32_t> error;
   UNKNOWN(4);
   be_val<uint32_t> pmc1;
   be_val<uint32_t> pmc2;
   be_val<uint32_t> pmc3;
   be_val<uint32_t> pmc4;
   be_val<uint32_t> mmcr0;
   be_val<uint32_t> mmcr1;
};
CHECK_OFFSET(OSContext, 0x00, tag);
CHECK_OFFSET(OSContext, 0x08, gpr);
CHECK_OFFSET(OSContext, 0x88, cr);
CHECK_OFFSET(OSContext, 0x8c, lr);
CHECK_OFFSET(OSContext, 0x90, ctr);
CHECK_OFFSET(OSContext, 0x94, xer);
CHECK_OFFSET(OSContext, 0x98, srr0);
CHECK_OFFSET(OSContext, 0x9c, srr1);
CHECK_OFFSET(OSContext, 0xb4, fpscr);
CHECK_OFFSET(OSContext, 0xb8, fpr);
CHECK_OFFSET(OSContext, 0x1b8, spinLockCount);
CHECK_OFFSET(OSContext, 0x1ba, state);
CHECK_OFFSET(OSContext, 0x1bc, gqr);
CHECK_OFFSET(OSContext, 0x1e0, psf);
CHECK_OFFSET(OSContext, 0x2e0, coretime);
CHECK_OFFSET(OSContext, 0x2f8, starttime);
CHECK_OFFSET(OSContext, 0x300, error);
CHECK_OFFSET(OSContext, 0x308, pmc1);
CHECK_OFFSET(OSContext, 0x30c, pmc2);
CHECK_OFFSET(OSContext, 0x310, pmc3);
CHECK_OFFSET(OSContext, 0x314, pmc4);
CHECK_OFFSET(OSContext, 0x318, mmcr0);
CHECK_OFFSET(OSContext, 0x31c, mmcr1);
CHECK_SIZE(OSContext, 0x320);

// refs: __OSDumpActiveThreads
namespace OSThreadState
{
enum Flags : uint8_t
{
   None     = 0,
   Ready    = 1 << 0,
   Running  = 1 << 1,
   Waiting  = 1 << 2,
   Moribund = 1 << 3 // define:Moribund "at the point of death."
};
}

enum class OSThreadRequest : uint32_t
{
   None = 0,
   Suspend = 1,
   Cancel = 2,
};

namespace OSThreadAttributes
{
enum Flags : uint8_t
{
   AffinityCPU0 = 1 << 0,
   AffinityCPU1 = 1 << 1,
   AffinityCPU2 = 1 << 2,
   AffinityAny = AffinityCPU0 | AffinityCPU1 | AffinityCPU2,
   Detached = 1 << 3,
   // ? 1 << 4
   StackUsage = 1 << 5,
};
}

struct OSMutex;

struct OSMutexQueue
{
   be_ptr<OSMutex> head;
   be_ptr<OSMutex> tail;
   be_ptr<void> parent;
   UNKNOWN(4);
};
CHECK_OFFSET(OSMutexQueue, 0x0, head);
CHECK_OFFSET(OSMutexQueue, 0x4, tail);
CHECK_OFFSET(OSMutexQueue, 0x8, parent);
CHECK_SIZE(OSMutexQueue, 0x10);

struct OSFastMutex;

struct OSFastMutexQueue
{
   be_ptr<OSFastMutex> head;
   be_ptr<OSFastMutex> tail;
};
CHECK_OFFSET(OSFastMutexQueue, 0x00, head);
CHECK_OFFSET(OSFastMutexQueue, 0x04, tail);
CHECK_SIZE(OSFastMutexQueue, 0x08);

struct OSThread
{
   static const uint32_t Tag = 0x74487244;

   OSContext context;
   be_val<uint32_t> tag;
   be_val<OSThreadState::Flags> state;
   be_val<OSThreadAttributes::Flags> attr; // OSSetThreadAffinity / OSCreateThread
   be_val<uint16_t> id;
   be_val<int32_t> suspendCounter;
   be_val<int32_t> priority;
   be_val<int32_t> basePriority;          // "ba" in DumpActiveThreads and returned in OSGetThreadPriority
   be_val<uint32_t> exitValue;            // Exit value of thread
   Fiber *fiber;                          // Naughty, hopefully not overriding anything important
   UNKNOWN(0x35c - 0x340);
   be_ptr<OSThreadQueue> queue;           // Queue the thread is on
   OSThreadLink link;                     // Thread queue link
   OSThreadQueue joinQueue;               // Queue of threads waiting to join this
   be_ptr<OSMutex> mutex;                 // Mutex we are waiting to lock
   OSMutexQueue mutexQueue;               // Mutexes owned by this thread
   OSThreadLink activeLink;               // Link on active thread queue
   be_val<uint32_t> stackStart;           // Stack starting value (top, highest address)
   be_val<uint32_t> stackEnd;             // Stack end value (bottom, lowest address)
   be_val<uint32_t> entryPoint;           // Entry point from OSCreateThread
   UNKNOWN(0x57c - 0x3a0);
   be_val<uint32_t> specific[0x10];
   UNKNOWN(0x5c0 - 0x5bc);
   be_ptr<const char> name;               // Thread name
   UNKNOWN(0x4);
   be_ptr<uint8_t> userStackPointer;      // The stack specified in OSCreateThread
   be_val<uint32_t> cleanupCallback;
   be_val<uint32_t> deallocator;
   be_val<uint32_t> cancelState;          // Is listening to requestFlag enabled
   be_val<OSThreadRequest> requestFlag;   // Request flag for cancel or suspend
   be_val<int32_t> needSuspend;           // How many pending suspends we have
   be_val<int32_t> suspendResult;         // Result of suspend
   OSThreadQueue suspendQueue;            // Queue of threads waiting for suspend to finish
   UNKNOWN(0x69c - 0x5f4);
};
CHECK_OFFSET(OSThread, 0x320, tag);
CHECK_OFFSET(OSThread, 0x324, state);
CHECK_OFFSET(OSThread, 0x325, attr);
CHECK_OFFSET(OSThread, 0x326, id);
CHECK_OFFSET(OSThread, 0x328, suspendCounter);
CHECK_OFFSET(OSThread, 0x32c, priority);
CHECK_OFFSET(OSThread, 0x330, basePriority);
CHECK_OFFSET(OSThread, 0x334, exitValue);
CHECK_OFFSET(OSThread, 0x35c, queue);
CHECK_OFFSET(OSThread, 0x360, link);
CHECK_OFFSET(OSThread, 0x368, joinQueue);
CHECK_OFFSET(OSThread, 0x378, mutex);
CHECK_OFFSET(OSThread, 0x37c, mutexQueue);
CHECK_OFFSET(OSThread, 0x38c, activeLink);
CHECK_OFFSET(OSThread, 0x394, stackStart);
CHECK_OFFSET(OSThread, 0x398, stackEnd);
CHECK_OFFSET(OSThread, 0x39c, entryPoint);
CHECK_OFFSET(OSThread, 0x57c, specific);
CHECK_OFFSET(OSThread, 0x5c0, name);
CHECK_OFFSET(OSThread, 0x5c8, userStackPointer);
CHECK_OFFSET(OSThread, 0x5cc, cleanupCallback);
CHECK_OFFSET(OSThread, 0x5d0, deallocator);
CHECK_OFFSET(OSThread, 0x5d4, cancelState);
CHECK_OFFSET(OSThread, 0x5d8, requestFlag);
CHECK_OFFSET(OSThread, 0x5dc, needSuspend);
CHECK_OFFSET(OSThread, 0x5e0, suspendResult);
CHECK_OFFSET(OSThread, 0x5e4, suspendQueue);
CHECK_SIZE(OSThread, 0x69c);

#pragma pack(pop)

using ThreadEntryPoint = wfunc_ptr<uint32_t, uint32_t, void*>;

void
OSCancelThread(OSThread *thread);

long
OSCheckActiveThreads();

int32_t
OSCheckThreadStackUsage(OSThread *thread);

void
OSClearThreadStackUsage(OSThread *thread);

void
OSContinueThread(OSThread *thread);

BOOL
OSCreateThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, void *argv, uint8_t *stack, uint32_t stackSize, int32_t priority, OSThreadAttributes::Flags attributes);

void
OSDetachThread(OSThread *thread);

void
OSExitThread(int value);

void
OSGetActiveThreadLink(OSThread *thread, OSThreadLink *link);

OSThread *
OSGetCurrentThread();

OSThread *
OSGetDefaultThread(uint32_t coreID);

uint32_t
OSGetStackPointer();

uint32_t
OSGetThreadAffinity(OSThread *thread);

const char *
OSGetThreadName(OSThread *thread);

uint32_t
OSGetThreadPriority(OSThread *thread);

uint32_t
OSGetThreadSpecific(uint32_t id);

BOOL
OSIsThreadSuspended(OSThread *thread);

BOOL
OSIsThreadTerminated(OSThread *thread);

BOOL
OSJoinThread(OSThread *thread, be_val<int> *val);

void
OSPrintCurrentThreadState();

int32_t
OSResumeThread(OSThread *thread);

BOOL
OSRunThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, p32<void> argv);

OSThread *
OSSetDefaultThread(uint32_t core, OSThread *thread);

BOOL
OSSetThreadAffinity(OSThread *thread, uint32_t affinity);

BOOL
OSSetThreadCancelState(BOOL state);

void
OSSetThreadName(OSThread* thread, const char *name);

BOOL
OSSetThreadPriority(OSThread* thread, uint32_t priority);

BOOL
OSSetThreadRunQuantum(OSThread* thread, uint32_t quantum);

void
OSSetThreadSpecific(uint32_t id, uint32_t value);

BOOL
OSSetThreadStackUsage(OSThread *thread);

void
OSSleepThread(OSThreadQueue *queue);

void
OSSleepTicks(OSTime ticks);

uint32_t
OSSuspendThread(OSThread *thread);

void
OSTestThreadCancel();

void
OSWakeupThread(OSThreadQueue *queue);

void
OSYieldThread();
