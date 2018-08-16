#pragma once
#include "sndcore2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::sndcore2
{

using AXFXAllocFuncPtr = virt_func_ptr<
   virt_ptr<void> (uint32_t size)
>;
using AXFXFreeFuncPtr = virt_func_ptr<
   void (virt_ptr<void> ptr)
>;

struct AXFXBuffers
{
   virt_ptr<int32_t> left;
   virt_ptr<int32_t> right;
   virt_ptr<int32_t> surround;
};
CHECK_OFFSET(AXFXBuffers, 0x00, left);
CHECK_OFFSET(AXFXBuffers, 0x04, right);
CHECK_OFFSET(AXFXBuffers, 0x08, surround);
CHECK_SIZE(AXFXBuffers, 0x0C);

struct AXFXChorus;
struct AXFXDelay;
struct AXFXReverbHi;
struct AXFXReverbStd;

void
AXFXSetHooks(AXFXAllocFuncPtr allocFn,
             AXFXFreeFuncPtr freeFn);

void
AXFXGetHooks(virt_ptr<AXFXAllocFuncPtr> allocFn,
             virt_ptr<AXFXFreeFuncPtr> freeFn);

int32_t
AXFXChorusExpGetMemSize(virt_ptr<AXFXChorus> chorus);

int32_t
AXFXDelayExpGetMemSize(virt_ptr<AXFXDelay> chorus);

int32_t
AXFXReverbHiExpGetMemSize(virt_ptr<AXFXReverbHi> chorus);

int32_t
AXFXReverbStdExpGetMemSize(virt_ptr<AXFXReverbStd> chorus);

void
AXFXChorusExpCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXChorus> data);

void
AXFXDelayExpCallback(virt_ptr<AXFXBuffers> buffers,
                     virt_ptr<AXFXDelay> data);

void
AXFXReverbHiExpCallback(virt_ptr<AXFXBuffers> buffers,
                        virt_ptr<AXFXReverbHi> data);

void
AXFXReverbStdExpCallback(virt_ptr<AXFXBuffers> buffers,
                         virt_ptr<AXFXReverbStd> data);

} // namespace cafe::sndcore2
