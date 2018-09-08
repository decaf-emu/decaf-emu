#pragma once
#include <common/platform_fiber.h>
#include <libcpu/cpu.h>
#include <libcpu/be2_struct.h>

namespace cafe::kernel
{

#ifndef DECAF_KERNEL_LLE
struct HostContext;
#endif

struct Context
{
   static constexpr uint64_t Tag = 0x4F53436F6E747874ull;

   //! Should always be set to the value OSContext::Tag.
   be2_val<uint64_t> tag;
   be2_array<uint32_t, 32> gpr;
   be2_val<uint32_t> cr;
   be2_val<uint32_t> lr;
   be2_val<uint32_t> ctr;
   be2_val<uint32_t> xer;

   be2_val<uint32_t> srr0;
   be2_val<uint32_t> srr1;

   //These are only set during an exception
   be2_val<uint32_t> dsisr;
   be2_val<uint32_t> dar;
   be2_val<uint32_t> exceptionType;

   UNKNOWN(0x8);

   be2_val<uint32_t> fpscr;
   be2_array<double, 32> fpr;
   be2_val<uint16_t> spinLockCount;
   be2_val<uint16_t> state;
   be2_array<uint32_t, 8> gqr;
   be2_val<uint32_t> pir;
   be2_array<double, 32> psf;
   be2_array<int64_t, 3> coretime;
   be2_val<int64_t> starttime;
   be2_val<int32_t> error;
   be2_val<uint32_t> attr;

#ifdef DECAF_KERNEL_LLE
   be2_val<uint32_t> pmc1;
   be2_val<uint32_t> pmc2;
   be2_val<uint32_t> pmc3;
   be2_val<uint32_t> pmc4;
#else
   HostContext *hostContext;
   be2_val<uint32_t> nia;
   be2_val<uint32_t> cia;
#endif

   be2_val<uint32_t> mmcr0;
   be2_val<uint32_t> mmcr1;
};
CHECK_OFFSET(Context, 0x00, tag);
CHECK_OFFSET(Context, 0x08, gpr);
CHECK_OFFSET(Context, 0x88, cr);
CHECK_OFFSET(Context, 0x8c, lr);
CHECK_OFFSET(Context, 0x90, ctr);
CHECK_OFFSET(Context, 0x94, xer);
CHECK_OFFSET(Context, 0x98, srr0);
CHECK_OFFSET(Context, 0x9c, srr1);
CHECK_OFFSET(Context, 0xA0, dsisr);
CHECK_OFFSET(Context, 0xA4, dar);
CHECK_OFFSET(Context, 0xA8, exceptionType);
CHECK_OFFSET(Context, 0xb4, fpscr);
CHECK_OFFSET(Context, 0xb8, fpr);
CHECK_OFFSET(Context, 0x1b8, spinLockCount);
CHECK_OFFSET(Context, 0x1ba, state);
CHECK_OFFSET(Context, 0x1bc, gqr);
CHECK_OFFSET(Context, 0x1DC, pir);
CHECK_OFFSET(Context, 0x1e0, psf);
CHECK_OFFSET(Context, 0x2e0, coretime);
CHECK_OFFSET(Context, 0x2f8, starttime);
CHECK_OFFSET(Context, 0x300, error);
#ifdef DECAF_KERNEL_LLE
CHECK_OFFSET(Context, 0x308, pmc1);
CHECK_OFFSET(Context, 0x30c, pmc2);
CHECK_OFFSET(Context, 0x310, pmc3);
CHECK_OFFSET(Context, 0x314, pmc4);
#endif
CHECK_OFFSET(Context, 0x318, mmcr0);
CHECK_OFFSET(Context, 0x31c, mmcr1);
CHECK_SIZE(Context, 0x320);

void
copyContextToCpu(virt_ptr<Context> context);

void
copyContextFromCpu(virt_ptr<Context> context);

void
exitThreadNoLock();

void
resetFaultedContextFiber(virt_ptr<Context> context,
                         platform::FiberEntryPoint entry,
                         void *param);

void
setContextFiberEntry(virt_ptr<Context> context,
                     platform::FiberEntryPoint entry,
                     void *param);

virt_ptr<Context>
getCurrentContext();

void
switchContext(virt_ptr<Context> next);

void
hijackCurrentHostContext(virt_ptr<Context> context);

void
sleepCurrentContext();

void
wakeCurrentContext();

namespace internal
{

void
initialiseCoreContext(cpu::Core *core);

void
initialiseStaticContextData();

} // namespace internal

} // namespace cafe::kernel
