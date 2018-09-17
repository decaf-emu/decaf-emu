#include "snduser2.h"
#include "snduser2_axfx.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::snduser2
{

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
   RegisterFunctionExport(AXFXReverbHiGetMemSize);
   RegisterFunctionExport(AXFXReverbHiExpGetMemSize);
   RegisterFunctionExport(AXFXReverbStdGetMemSize);
   RegisterFunctionExport(AXFXReverbStdExpGetMemSize);
   RegisterFunctionExport(AXFXReverbHiCallback);
   RegisterFunctionExport(AXFXReverbHiExpCallback);
   RegisterFunctionExport(AXFXMultiChReverbCallback);
   RegisterFunctionExport(AXFXReverbStdCallback);
   RegisterFunctionExport(AXFXReverbStdExpCallback);
   RegisterFunctionExport(AXFXReverbHiInit);
   RegisterFunctionExport(AXFXReverbHiExpInit);
   RegisterFunctionExport(AXFXReverbStdInit);
   RegisterFunctionExport(AXFXReverbStdExpInit);
   RegisterFunctionExport(AXFXReverbHiShutdown);
   RegisterFunctionExport(AXFXReverbHiExpShutdown);
   RegisterFunctionExport(AXFXReverbStdShutdown);
   RegisterFunctionExport(AXFXReverbStdExpShutdown);
   RegisterFunctionExport(AXFXReverbHiSettings);
   RegisterFunctionExport(AXFXMultiChReverbInit);
   RegisterFunctionExport(AXARTServiceSounds);
   RegisterFunctionExport(SPGetSoundEntry);
}

} // namespace cafe::snduser2
