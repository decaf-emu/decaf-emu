#include "sndcore2.h"
#include "sndcore2_device.h"
#include "sndcore2_fx.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::sndcore2
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
AXFXGetHooks(virt_ptr<AXFXAllocFuncPtr> allocFn,
             virt_ptr<AXFXFreeFuncPtr> freeFn)
{
   *allocFn = sAXFXMemAlloc;
   *freeFn = sAXFXMemFree;
}

int32_t
AXFXChorusGetMemSize(virt_ptr<AXFXChorus> chorus)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXChorusExpGetMemSize(virt_ptr<AXFXChorus> chorus)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXDelayGetMemSize(virt_ptr<AXFXDelay> chorus)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXDelayExpGetMemSize(virt_ptr<AXFXDelay> chorus)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXReverbHiGetMemSize(virt_ptr<AXFXReverbHi> chorus)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXReverbHiExpGetMemSize(virt_ptr<AXFXReverbHi> chorus)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXReverbStdGetMemSize(virt_ptr<AXFXReverbStd> chorus)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXReverbStdExpGetMemSize(virt_ptr<AXFXReverbStd> chorus)
{
   decaf_warn_stub();
   return 32;
}

void
AXFXChorusCallback(virt_ptr<AXFXBuffers> buffers,
                   virt_ptr<AXFXChorus> data,
                   virt_ptr<AXAuxCallbackData> auxData)
{
   decaf_warn_stub();
}

void
AXFXChorusExpCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXChorus> data)
{
   decaf_warn_stub();
}

void
AXFXDelayCallback(virt_ptr<AXFXBuffers> buffers,
                  virt_ptr<AXFXDelay> data,
                  virt_ptr<AXAuxCallbackData> auxData)
{
   decaf_warn_stub();
}

void
AXFXDelayExpCallback(virt_ptr<AXFXBuffers> buffers,
                     virt_ptr<AXFXDelay> data)
{
   decaf_warn_stub();
}

void
AXFXReverbHiCallback(virt_ptr<AXFXBuffers> buffers,
                     virt_ptr<AXFXReverbHi> data,
                     virt_ptr<AXAuxCallbackData> auxData)
{
   decaf_warn_stub();
}

void
AXFXReverbHiExpCallback(virt_ptr<AXFXBuffers> buffers,
                        virt_ptr<AXFXReverbHi> data)
{
   decaf_warn_stub();
}

void
AXFXReverbStdCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbStd> data,
                      virt_ptr<AXAuxCallbackData> auxData)
{
   decaf_warn_stub();
}

void
AXFXReverbStdExpCallback(virt_ptr<AXFXBuffers> buffers,
                         virt_ptr<AXFXReverbStd> data)
{
   decaf_warn_stub();
}

void
Library::registerFxSymbols()
{
   RegisterFunctionExport(AXFXSetHooks);
   RegisterFunctionExport(AXFXGetHooks);
   RegisterFunctionExport(AXFXChorusGetMemSize);
   RegisterFunctionExport(AXFXChorusExpGetMemSize);
   RegisterFunctionExport(AXFXDelayGetMemSize);
   RegisterFunctionExport(AXFXDelayExpGetMemSize);
   RegisterFunctionExport(AXFXReverbHiGetMemSize);
   RegisterFunctionExport(AXFXReverbHiExpGetMemSize);
   RegisterFunctionExport(AXFXReverbStdGetMemSize);
   RegisterFunctionExport(AXFXReverbStdExpGetMemSize);
   RegisterFunctionExport(AXFXChorusCallback);
   RegisterFunctionExport(AXFXChorusExpCallback);
   RegisterFunctionExport(AXFXDelayCallback);
   RegisterFunctionExport(AXFXDelayExpCallback);
   RegisterFunctionExport(AXFXReverbHiCallback);
   RegisterFunctionExport(AXFXReverbHiExpCallback);
   RegisterFunctionExport(AXFXReverbStdCallback);
   RegisterFunctionExport(AXFXReverbStdExpCallback);
}

} // namespace cafe::sndcore2
