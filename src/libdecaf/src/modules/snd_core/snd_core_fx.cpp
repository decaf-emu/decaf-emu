#include "snd_core.h"
#include "snd_core_fx.h"

namespace snd_core
{

static AXFXAllocFuncPtr
sAXFXMemAlloc = nullptr;

static AXFXFreeFuncPtr
sAXFXMemFree = nullptr;

void
AXFXSetHooks(AXFXAllocFuncPtr allocFn,
             AXFXFreeFuncPtr freeFn)
{
   sAXFXMemAlloc = allocFn;
   sAXFXMemFree = freeFn;
}

void
AXFXGetHooks(be_AXFXAllocFuncPtr *allocFn,
             be_AXFXFreeFuncPtr *freeFn)
{
   *allocFn = sAXFXMemAlloc;
   *freeFn = sAXFXMemFree;
}

int32_t
AXFXChorusExpGetMemSize(AXFXChorus *chorus)
{
   decaf_warn_stub();

   return 32;
}

int32_t
AXFXDelayExpGetMemSize(AXFXDelay *chorus)
{
   decaf_warn_stub();

   return 32;
}

int32_t
AXFXReverbHiExpGetMemSize(AXFXReverbHi *chorus)
{
   decaf_warn_stub();

   return 32;
}

int32_t
AXFXReverbStdExpGetMemSize(AXFXReverbStd *chorus)
{
   decaf_warn_stub();

   return 32;
}

void
AXFXChorusExpCallback(AXFXBuffers *buffers, AXFXChorus *data)
{
   decaf_warn_stub();
}

void
AXFXDelayExpCallback(AXFXBuffers *buffers, AXFXDelay *data)
{
   decaf_warn_stub();
}

void
AXFXReverbHiExpCallback(AXFXBuffers *buffers, AXFXReverbHi *data)
{
   decaf_warn_stub();
}

void
AXFXReverbStdExpCallback(AXFXBuffers *buffers, AXFXReverbStd *data)
{
   decaf_warn_stub();
}

void
Module::registerFXFunctions()
{
   RegisterKernelFunction(AXFXSetHooks);
   RegisterKernelFunction(AXFXGetHooks);
   RegisterKernelFunction(AXFXChorusExpGetMemSize);
   RegisterKernelFunction(AXFXDelayExpGetMemSize);
   RegisterKernelFunction(AXFXReverbHiExpGetMemSize);
   RegisterKernelFunction(AXFXReverbStdExpGetMemSize);
   RegisterKernelFunction(AXFXChorusExpCallback);
   RegisterKernelFunction(AXFXDelayExpCallback);
   RegisterKernelFunction(AXFXReverbHiExpCallback);
   RegisterKernelFunction(AXFXReverbStdExpCallback);
}

} // namespace snd_core
