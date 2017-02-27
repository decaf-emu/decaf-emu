#pragma once
#include "snd_core_enum.h"
#include "ppcutils/wfunc_ptr.h"

#include <common/be_val.h>
#include <cstdint>

namespace snd_core
{

using AXFXAllocFuncPtr = wfunc_ptr<void *, uint32_t>;
using AXFXFreeFuncPtr = wfunc_ptr<void, void *>;
using be_AXFXAllocFuncPtr = be_wfunc_ptr<void *, uint32_t>;
using be_AXFXFreeFuncPtr = be_wfunc_ptr<void, void *>;

struct AXFXBuffers
{
   int32_t *left;
   int32_t *right;
   int32_t *surround;
};

struct AXFXChorus;
struct AXFXDelay;
struct AXFXReverbHi;
struct AXFXReverbStd;

void
AXFXSetHooks(AXFXAllocFuncPtr allocFn,
             AXFXFreeFuncPtr freeFn);

void
AXFXGetHooks(be_AXFXAllocFuncPtr *allocFn,
             be_AXFXFreeFuncPtr *freeFn);

int32_t
AXFXChorusExpGetMemSize(AXFXChorus *chorus);

int32_t
AXFXDelayExpGetMemSize(AXFXDelay *chorus);

int32_t
AXFXReverbHiExpGetMemSize(AXFXReverbHi *chorus);

int32_t
AXFXReverbStdExpGetMemSize(AXFXReverbStd *chorus);

void
AXFXChorusExpCallback(AXFXBuffers *buffers, AXFXChorus *data);

void
AXFXDelayExpCallback(AXFXBuffers *buffers, AXFXDelay *data);

void
AXFXReverbHiExpCallback(AXFXBuffers *buffers, AXFXReverbHi *data);

void
AXFXReverbStdExpCallback(AXFXBuffers *buffers, AXFXReverbStd *data);

} // namespace snd_core
