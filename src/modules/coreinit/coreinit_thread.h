#pragma once
#include "systemtypes.h"
#include "coreinit_time.h"
#include "util.h"

#pragma pack(push, 1)

struct OSThread;
class Thread;

struct OSThreadQueue
{
   be_ptr<OSThread> head;
   be_ptr<OSThread> tail;
   be_ptr<void> parent;
   UNKNOWN(4);
};
CHECK_OFFSET(OSThreadQueue, 0x00, head);
CHECK_OFFSET(OSThreadQueue, 0x04, tail);
CHECK_OFFSET(OSThreadQueue, 0x08, parent);
CHECK_SIZE(OSThreadQueue, 0x10);

// OSDumpContext, OSLoadFPUContext, OSSaveContext
// OSGetLastError
// 0200F060 crash dump handler
// OSCheckActiveThreads 0x201AAA0
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
CHECK_OFFSET(OSContext, 0x1bc, gqr)
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

enum class OSThreadRequestFlag : uint32_t
{
   None = 0,
   Suspend = 1,
   Cancel = 2,
};

namespace OSThreadAttributes
{
enum Flags : uint8_t
{
   AffinityNone = 7, // If affinity_none is 7 = 0b111 then lets assume its a bitmask for now
   AffinityCPU0 = 1,
   AffinityCPU1 = 2,
   AffinityCPU2 = 4,
};
}

struct OSThread
{
   static const uint32_t Tag = 0x74487244;

   OSContext context;
   be_val<uint32_t> tag;
   be_val<OSThreadState::Flags> state;
   be_val<OSThreadAttributes::Flags> attr; // OSSetThreadAffinity / OSCreateThread
   be_val<uint16_t> id;
   be_val<int32_t> suspendCounter;
   be_val<uint32_t> priority;
   be_val<uint32_t> basePriority; // "ba" in DumpActiveThreads and returned in OSGetThreadPriority
   be_val<uint32_t> exitValue; // OSExitThread / OSJoinThread
   Thread *thread; // THIS IS CUSTOM! squueeezed in, hopefully not override anything important.
   UNKNOWN(0x394 - 0x340);
   be_val<uint32_t> stackStart; // __OSSaveUserStackPointer
   be_val<uint32_t> stackEnd; // __OSSaveUserStackPointer
   be_val<uint32_t> entryPoint; // OSCreateThread
   UNKNOWN(0x57c - 0x3a0);
   be_val<uint32_t> specific[0x10]; // OSGetThreadSpecific
   UNKNOWN(0x5c0 - 0x5bc);
   be_ptr<char> name; // OSGetThreadName
   UNKNOWN(0x4);
   be_val<uint32_t> userStackPointer; // OSGetUserStackPointer
   be_val<uint32_t> cleanupCallback; // OSSetThreadCleanupCallback
   be_val<uint32_t> deallocator; // OSSetThreadDeallocator
   be_val<uint32_t> cancelState; // OSSetThreadCancelState
   be_val<uint32_t> requestFlag;
   UNKNOWN(0x69c - 0x5dc);
};
CHECK_OFFSET(OSThread, 0x320, tag);
CHECK_OFFSET(OSThread, 0x324, state);
CHECK_OFFSET(OSThread, 0x325, attr);
CHECK_OFFSET(OSThread, 0x326, id);
CHECK_OFFSET(OSThread, 0x328, suspendCounter);
CHECK_OFFSET(OSThread, 0x32c, priority);
CHECK_OFFSET(OSThread, 0x330, basePriority);
CHECK_OFFSET(OSThread, 0x334, exitValue);
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
CHECK_SIZE(OSThread, 0x69c);

#pragma pack(pop)

using ThreadEntryPoint = wfunc_ptr<uint32_t, uint32_t, void*>;

OSThread *
OSGetCurrentThread();

BOOL
OSCreateThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, void *argv, uint8_t *stack, uint32_t stackSize, uint32_t priority, OSThreadAttributes::Flags attributes);

void
OSSetDefaultThread(uint32_t coreID, OSThread *thread);

OSThread *
OSGetDefaultThread(uint32_t coreID);

void
OSInitThreadQueue(OSThreadQueue *queue);

void
OSInitThreadQueueEx(OSThreadQueue *queue, p32<void> parent);

BOOL
OSSetThreadPriority(OSThread *thread, uint32_t priority);

uint32_t
OSGetThreadPriority(OSThread *thread);

BOOL
OSSetThreadAffinity(OSThread *thread, uint32_t affinity);

uint32_t
OSGetThreadAffinity(OSThread *thread);

uint32_t
OSGetThreadSpecific(uint32_t id);

void
OSSetThreadSpecific(uint32_t id, uint32_t value);

void
OSSetThreadName(OSThread *thread, const char *name);

const char *
OSGetThreadName(OSThread *thread);

void
OSSleepTicks(Time ticks);

uint32_t
OSResumeThread(OSThread *thread);

BOOL
OSJoinThread(OSThread *thread, be_val<int> *exitValue);

uint32_t
OSSuspendThread(OSThread *thread);

void
OSYieldThread();
