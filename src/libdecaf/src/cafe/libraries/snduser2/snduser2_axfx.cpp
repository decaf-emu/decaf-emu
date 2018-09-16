#include "snduser2.h"
#include "snduser2_axfx.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::snduser2
{

struct StaticFxData
{
   be2_val<AXFXAllocFuncPtr> allocFuncPtr;
   be2_val<AXFXFreeFuncPtr> freeFuncPtr;
};

static virt_ptr<StaticFxData>
sFxData = nullptr;

void
AXFXSetHooks(AXFXAllocFuncPtr allocFn,
             AXFXFreeFuncPtr freeFn)
{
   sFxData->allocFuncPtr = allocFn;
   sFxData->freeFuncPtr = freeFn;
}

void
AXFXGetHooks(virt_ptr<AXFXAllocFuncPtr> allocFn,
             virt_ptr<AXFXFreeFuncPtr> freeFn)
{
   *allocFn = sFxData->allocFuncPtr;
   *freeFn = sFxData->freeFuncPtr;
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
AXFXDelayGetMemSize(virt_ptr<AXFXDelay> delay)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXDelayExpGetMemSize(virt_ptr<AXFXDelay> delay)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXReverbHiGetMemSize(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXReverbHiExpGetMemSize(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXReverbStdGetMemSize(virt_ptr<AXFXReverbStd> reverb)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXReverbStdExpGetMemSize(virt_ptr<AXFXReverbStd> reverb)
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
AXFXMultiChReverbCallback(virt_ptr<AXFXBuffers> buffers,
                          virt_ptr<AXFXReverbMulti> data,
                          virt_ptr<AXAuxCallbackData> auxData)
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

BOOL
AXFXChorusInit(virt_ptr<AXFXChorus> chorus)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXChorusExpInit(virt_ptr<AXFXChorus> chorus)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXDelayInit(virt_ptr<AXFXDelay> delay)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXDelayExpInit(virt_ptr<AXFXDelay> delay)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXReverbHiInit(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXReverbHiExpInit(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXReverbStdInit(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXReverbStdExpInit(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

void
AXFXChorusShutdown(virt_ptr<AXFXChorus> chorus)
{
   decaf_warn_stub();
}

void
AXFXChorusExpShutdown(virt_ptr<AXFXChorus> chorus)
{
   decaf_warn_stub();
}

void
AXFXDelayShutdown(virt_ptr<AXFXDelay> delay)
{
   decaf_warn_stub();
}

void
AXFXDelayExpShutdown(virt_ptr<AXFXDelay> delay)
{
   decaf_warn_stub();
}

BOOL
AXFXReverbHiShutdown(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

void
AXFXReverbHiExpShutdown(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
}

void
AXFXReverbStdShutdown(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
}

void
AXFXReverbStdExpShutdown(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
}

BOOL
AXFXReverbHiSettings(virt_ptr<AXFXReverbHi> reverb)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXMultiChReverbInit(virt_ptr<AXFXReverbMulti> reverb,
                      uint32_t unk,
                      AXFXSampleRate sampleRate)
{
   decaf_warn_stub();
   return TRUE;
}

void
AXARTServiceSounds()
{
}

virt_ptr<void>
SPGetSoundEntry(virt_ptr<void> table, uint32_t index)
{
   decaf_warn_stub();
   return NULL;
}

void
Library::registerAxfxSymbols()
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
   RegisterFunctionExport(AXFXMultiChReverbCallback);
   RegisterFunctionExport(AXFXReverbStdCallback);
   RegisterFunctionExport(AXFXReverbStdExpCallback);
   RegisterFunctionExport(AXFXChorusInit);
   RegisterFunctionExport(AXFXChorusExpInit);
   RegisterFunctionExport(AXFXDelayInit);
   RegisterFunctionExport(AXFXDelayExpInit);
   RegisterFunctionExport(AXFXReverbHiInit);
   RegisterFunctionExport(AXFXReverbHiExpInit);
   RegisterFunctionExport(AXFXReverbStdInit);
   RegisterFunctionExport(AXFXReverbStdExpInit);
   RegisterFunctionExport(AXFXChorusShutdown);
   RegisterFunctionExport(AXFXChorusExpShutdown);
   RegisterFunctionExport(AXFXDelayShutdown);
   RegisterFunctionExport(AXFXDelayExpShutdown);
   RegisterFunctionExport(AXFXReverbHiShutdown);
   RegisterFunctionExport(AXFXReverbHiExpShutdown);
   RegisterFunctionExport(AXFXReverbStdShutdown);
   RegisterFunctionExport(AXFXReverbStdExpShutdown);
   RegisterFunctionExport(AXFXReverbHiSettings);
   RegisterFunctionExport(AXFXMultiChReverbInit);
   RegisterFunctionExport(AXARTServiceSounds);
   RegisterFunctionExport(SPGetSoundEntry);

   RegisterDataInternal(sFxData);
}

} // namespace cafe::snduser2
