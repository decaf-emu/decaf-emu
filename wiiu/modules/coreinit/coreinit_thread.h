#pragma once
#include "systemtypes.h"

#pragma pack(push, 1)

struct OSThread;

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
enum OS_THREAD_STATE : uint8_t
{
   OS_THREAD_STATE_NONE       = 0,
   OS_THREAD_STATE_READY      = 1,
   OS_THREAD_STATE_RUNNING    = 2,
   OS_THREAD_STATE_WAITING    = 4,
   OS_THREAD_STATE_MORIBUND   = 8
};

enum OS_THREAD_REQUEST_FLAG : uint32_t
{
   OS_THREAD_REQUEST_FLAG_NONE      = 0,
   OS_THREAD_REQUEST_FLAG_SUSPEND   = 1,
   OS_THREAD_REQUEST_FLAG_CANCEL    = 2,
};

struct OSThread
{
   OSContext context;
   be_val<uint32_t> tag;
   be_val<OS_THREAD_STATE> state;
   be_val<uint8_t> attr;
   UNKNOWN(2);//326
   UNKNOWN(4);//suspend 328
   be_val<uint32_t> priority;
   be_val<uint32_t> basePriority; // "ba" in DumpActiveThreads and returned in OSGetThreadPriority
   UNKNOWN(0x57c - 0x334);
   be_val<uint32_t> specific[0x10]; // OSGetThreadSpecific
   UNKNOWN(0x5c0 - 0x5bc);
   be_ptr<char> name; // OSGetThreadName
   UNKNOWN(0x5d4 - 0x5c4);
   be_val<uint32_t> cancelState; // OSSetThreadCancelState
   UNKNOWN(0x69c - 0x5d8)
};
CHECK_OFFSET(OSThread, 0x320, tag);
CHECK_OFFSET(OSThread, 0x324, state);
CHECK_OFFSET(OSThread, 0x325, attr);
CHECK_OFFSET(OSThread, 0x32c, priority);
CHECK_OFFSET(OSThread, 0x330, basePriority);
CHECK_OFFSET(OSThread, 0x57c, specific);
CHECK_OFFSET(OSThread, 0x5c0, name);
CHECK_OFFSET(OSThread, 0x5d4, cancelState);
CHECK_SIZE(OSThread, 0x69c);

#pragma pack(pop)

p32<OSThread>
OSGetCurrentThread();

void
OSInitThreadQueue(p32<OSThreadQueue> pQueue);

void
OSInitThreadQueueEx(p32<OSThreadQueue> pQueue, p32<void> pParent);
